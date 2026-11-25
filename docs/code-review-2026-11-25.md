# lcy 代码审查报告

- **日期**：2026-11-25
- **审查对象**：仓库 `lcy`，HEAD `96a09bf`（修复不一致加锁顺序导致的死锁风险；`0d6f36c` 已修复循环引用）+ 工作区未提交改动（`lcy/asio/detail/impl/scheduler.ipp` 中对 C1 的部分修复）
- **范围**：`lcy/` 下 148 个文件，约 11,600 行（asio 核心/网络/SSL、bytra HTTP、log4cc、system/object_pool）
- **方法**：按模块并行深审；每条结论回到源码核对；关键项用独立程序实测复现（复现记录见第 7 节）
- **本文档只记录问题，不包含代码修改**

---

## 1. 结论摘要

单线程、同步 socket、正常 I/O 路径的质量不错；`flat_buffer`、HTTP parser 的核心状态机、plain socket 的 syscall 处理基本正确。但以下四个区域存在结构性缺陷，**当前状态不适合直接暴露给不可信对端或多线程生产负载**：

1. **scheduler 唤醒协议**：存在可复现的丢失唤醒，以及单线程模式的永久睡眠。
2. **operation 生命周期**：投机投递绕过取消、descriptor 状态池化复用、同方向 op 覆盖。
3. **HTTP 分帧**：CL/TE 处理可被用于请求走私，响应不区分状态码。
4. **SSL 引擎**：读写共用 scratch buffer、无序列化。

| 级别 | 数量 | 含义 |
|---|---|---|
| Critical | 9 | 正常使用即可导致挂起、错误的连接行为、UAF 或安全问题 |
| Important | 26 | 特定但合理的用法下触发；会造成资源泄漏、错误语义、竞争或数据损坏 |
| Minor | 8 | 边缘情况、健壮性或 API 一致性问题 |

> 严重级别的划分为审查者判断，业务场景（例如 parser 是否面向不可信输入、是否多线程）会改变实际优先级。
>
> 注：工作区未提交的 `scheduler.ipp` 改动已基本修复 C1；其余 8 条 Critical 在 HEAD `96a09bf` 上仍然存在。

---

## 2. Critical

### C1. scheduler：post 可能永久丢失唤醒

- 位置：`lcy/asio/detail/impl/scheduler.ipp:44-59`、`135-149`
- 描述：run 线程弹出 event token 后先释放 `op_queue_mutex_`（第 46 行），之后才设置 `block_on_reactor_ = true`（第 55 行）并进入 `epoll_wait(-1)`。poster 在这个窗口内推入 op 并调用 `wake_up_one_thread`，会读到 `block_on_reactor_ == false` 而跳过 `reactor_->wake_up()`。此后 run 线程在 epoll 中永久阻塞，op 留在 `op_queue_` 里无人处理。
- 触发场景：多线程 `io_context`，run 线程正在 token → reactor 的切换间隙，另一线程 post。
- 验证：代码路径确认；reviewer 以循环 post + 2s 超时复现（iter 7）。
- **当前状态**：工作区有一处未提交的修复，把 `timeout` 的计算和 `block_on_reactor_ = true` 移到 `op_queue_mutex_.unlock()` 之前。该改动关闭了丢失唤醒窗口（poster 在释放锁之后一定能看到标志），已通过构建与 `scheduler_test` / `strand_test` / `tcp_socket_test`。残留小问题：置位被提到了 `event_token_` 分支之外，普通 op 也会把标志留为 `true` 且不会立即复位，会产生多余的 eventfd 唤醒（无害但不必要）；建议只在 `event_token_` 分支、解锁之前置位。
- 修复建议（若不走上述方案）：reactor 存在时无条件 `reactor_->wake_up()`（eventfd 写入开销很小，伪唤醒只会让循环重跑）。

### C2. scheduler：首次创建 reactor 不唤醒 condvar 等待者

- 位置：`lcy/asio/detail/impl/scheduler.ipp:127-133`（`init_reactor`）
- 描述：`init_reactor` 在 `op_queue_mutex_` 下推入 event token，但没有 `notify`；`submit_private_op_queue` 则会 notify。若 run 线程先启动并睡在 `condvar_.wait`，之后才第一次创建 timer/socket（触发 `get_reactor`），token 无人处理，timerfd 已 armed 但没有线程进入 `epoll_wait`，定时器/首个 accept/read 永不触发。
- 触发场景：先启动线程池，再创建 acceptor 或 timer —— 非常常见的服务启动顺序。
- 验证：**实测复现**。2 个 run 线程 + work_guard，150ms 后创建 timer 并 `expires_after(50)` + `async_wait`，600ms 后 `fired=0`；手动 `post` 一次后 `fired=1`。
- 修复建议：push token 后 `condvar_.notify_one()`（注意此处已持有 `reactor_mutex_`，不要调用 `wake_up_one_thread`）。

### C3. `concurrency_hint == 1` 关闭全部同步，且可能永久睡眠

- 位置：`lcy/asio/detail/conditionally_condition_variable.hpp:29-40`（`::sleep(UINT_MAX)`）、`conditionally_mutex.hpp:22-34`、`scheduler.ipp:17`、`strand_service.hpp:92/143`、`reactor.ipp:27-29`、`service_repository.hpp:23`
- 描述：`concurrency_hint` 被当作“无需加锁”的开关。但跨线程 `post`、`strand::post`、service 创建在该模式下仍然合法且无同步，op 队列、strand 链表、descriptor 状态、service 仓库都可能竞争。更严重的是 condvar 退化为 `::sleep(UINT_MAX)`：单线程 `io_context` 配 `work_guard` 且尚无服务时，`run()` 会永久睡眠，`post()`/`stop()` 都无法唤醒。
- 触发场景：默认 `io_context` 在单核机器上 `hardware_concurrency() == 1`；或用户显式构造 `io_context(1)` 后使用 work_guard。
- 验证：**实测复现**。`io_context(1)` + `work_guard`，200ms 后 `run returned=0`，调用 `stop()` 再等 300ms 仍为 0。
- 修复建议：op 队列、strand、descriptor 状态等跨线程可见对象始终使用真实锁/条件变量；`concurrency_hint` 只作为容量/线程数提示。绝不要用 `sleep(UINT_MAX)` 代替 condvar。

### C4. ET 边沿可能被吞掉，导致读写永久挂起

- 位置：`lcy/asio/detail/impl/reactor.ipp:162-214`（`start_op`）、`455-485`（`handle_descriptor_events`）
- 描述：fd 在 `assign` 时以 `EPOLLIN | EPOLLPRI | EPOLLET` 注册一次，读方向之后不再 `EPOLL_CTL_MOD`。`start_op` 先做一次非阻塞 `poll()`；若未就绪，才在 ds 锁内登记 op。若数据在 poll 返回 0 之后、登记之前到达，reactor 线程会取到 EPOLLIN 边沿，但此时 `ds->events_` 还没有 read 位，事件被直接丢弃；EPOLLET 不会再报，op 永久等待。写方向因为 `registered_events_` 保留 EPOLLOUT，重新 armed 时也不会 MOD，同样存在窗口。
- 触发场景：高负载下 read/accept 的发起与数据到达交错。
- 验证：代码确认；reviewer 用内核行为探针确认“边沿被消费后 epoll_wait 不再返回，但 EPOLL_CTL_MOD 会重新评估就绪”。
- 修复建议：每次 `start_op` 都对该 fd 做 `EPOLL_CTL_MOD`（内核会重新检查当前就绪并产生事件），或改用 `EPOLLONESHOT` 并在每次发起时 MOD。

### C5. 投机投递的 operation 绕过 close/cancel 取消

- 位置：`lcy/asio/detail/impl/reactor.ipp:166-183`；调用方 `socket_service_base.hpp:359/378`、`stream_socket_service.hpp:112`、`datagram_socket_service.hpp:118/141`
- 描述：`start_op(..., try_speculative=true)` 在 `poll()` 就绪时直接 `sch_->post(std::move(op))`，既不保存到 `ds->read_op_/write_op_`，也不设置 `ds->events_`。`close()` 通过 `cancel_ops` 只能取消 ds 中登记过的操作，因此这个已投递的 op 无法被取消。
- 触发场景：`async_write_some` 后立即 `close()`/析构 socket；此时写 op 通常已经通过 speculative poll 投递。
- 验证：**实测复现**。connected socket 上 `async_write_some` 后立即 `close()`，handler 收到 `Bad file descriptor` 而不是 `operation_aborted`。
- 影响：fd 被复用时会操作到别的 socket；若 socket 对象先于 `run()` 销毁，op 中的 `impl_` 裸指针悬空 → UAF。
- 修复建议：speculative 投递也登记到 ds（需要一个“已投递但未完成”的槽位）；或在 operation 执行前于 ds 锁内复核 `is_open`/fd 身份，close 时标记 abort。

### C6. SSL 读写共用同一块 scratch buffer，且没有序列化

- 位置：`lcy/asio/ssl/detail/stream_socket_core.hpp:45`（`char* buffer_`）、`377-406`（fill/flush 同步）、`471-519`（fill/flush 异步）
- 描述：`async_flush_write_bio` 把 `write_bio_` 的数据读进 `impl.buffer_`，然后把指向该 buffer 的 view 交给 `async_write_some`；在发送完成之前，读路径的 `async_fill_read_bio` / 再次 `async_flush_write_bio` 会写入同一块内存，覆盖尚未发出的 TLS 密文。另外，所有 `SSL_read/SSL_write/SSL_connect/SSL_accept` 都在调用线程直接执行，没有任何 strand/mutex；多线程 `io_context` 下会对同一个 `SSL*`/`BIO*` 并发调用。
- 触发场景：全双工使用（同时有 pending read 和 write）即可触发；多线程下更严重。
- 验证：代码确认（未做真实 TLS 压测）。
- 修复建议：读写各用独立 buffer（参考 Boost.Asio ssl engine 的 `read_buffer_` / `write_buffer_`）；用 strand 或每 stream 的 mutex 串行化 SSL 引擎调用。

### C7. HTTP 分帧可被用于请求走私（CL.CL / TE.CL / TE+CL）

- 位置：`lcy/bytra/http/detail/impl/basic_parser.ipp:484-500`
- 描述：
  - `Content-Length` 通过 `get_field` 只取第一条，重复/冲突的 CL 被静默接受；
  - 只检查第一条 `Transfer-Encoding`，用 `token_list_contains` 判断是否包含 `chunked`，而不是要求最后一个 coding 为 chunked；
  - `Transfer-Encoding: gzip` 等非 chunked 的 TE 会回退到 `Content-Length`；
  - TE + CL 并存时接受 TE、保留 CL（`header_to_string` 还会把两者一起重新序列化）。
- 触发场景：parser 面向不可信请求/响应；前端代理与后端使用不同的分帧规则。
- 验证：**实测复现**。
  - 两条冲突 `Content-Length: 5` / `6` → `ec=Success`（静默取 5）；
  - `TE: gzip` + `CL: 5` → 按 CL 分帧，接受；
  - `TE: gzip` + `TE: chunked` + `CL: 5` → header+5 字节被消费，**剩余 10 字节被当作下一个请求**。
- 修复建议：合并所有 TE 值；请求要求最后一个 coding 为 `chunked`；拒绝重复/冲突 CL（除非所有值一致）；拒绝 TE 与 CL 并存；chunked 消息重新序列化时移除 CL。

### C8. HTTP 响应分帧忽略状态码 / HEAD

- 位置：`lcy/bytra/http/detail/impl/basic_parser.ipp:502-515`
- 描述：`parse_fields` 只要有 `content_length > 0` 就进入 body 阶段，不区分响应状态。`1xx`、`204`、`304` 永远没有 body（304 常带表示层的 `Content-Length`），`HEAD` 响应也没有 body 但 parser 不知道请求方法。
- 触发场景：客户端复用连接、或收到 304/204/HEAD 响应。
- 验证：**实测复现**。`HTTP/1.1 304 Not Modified\r\nContent-Length: 10\r\n\r\nGET / HTTP/` → body 被填成 `GET / HTTP`，后续响应被吃掉。
- 修复建议：响应解析时按状态码强制无 body；为 HEAD 增加显式标志；`1xx` 作为完整消息返回，交给调用方继续解析。

### C9. log4cc async sink：排队 handler 捕获裸 `this`，无析构处理，`flush()` 为空

- 位置：`lcy/log4cc/detail/basic_async_tcp_sink.hpp:51-65/67-78/80-83/85-114`
- 描述：构造函数、`reload()`、`log()` 都把捕获 `[this]` 的 lambda post 到 strand/scheduler；类没有 `enable_shared_from_this`，没有析构函数取消/等待在途操作，`flush()` 是空实现。
- 触发场景：sink/logger 先于 `io_context` 销毁，但仍有排队的状态机 handler；或 `flush()` 后立即销毁 logger。
- 影响：handler 解引用已释放的 `this`（UAF）；已格式化但未发送的消息在销毁时丢失。
- 修复建议：`enable_shared_from_this` + 捕获 `weak_ptr`/`shared_ptr`；析构时取消并（可选）drain；实现真正的 `flush()`（post 一个完成通知并等待，或明确文档化为不保证 flush）。

---

## 3. Important

### 3.1 asio 核心

| ID | 位置 | 问题 | 建议 |
|---|---|---|---|
| I1 | `scheduler.ipp:32-35,103-115` | 没有工作后 `operation_decrease()` 调用 `stop()`，`stopped_` 永不复位且无 restart API；之后 `post` 入队但 `run` 永不执行。**实测 `a=1,b=0`** | 区分“空闲”与“stop”；提供 restart，或在 submit 时（持锁）清除 stopped |
| I2 | `reactor.ipp:72-112,129-133` + `object_pool.hpp` | `epoll_wait` 返回的事件持有 `internal_descriptor_state*`；另一线程 close 后 ds 被回收，新 socket 复用同一地址，旧事件会错误完成新 socket 的 op | EPOLLONESHOT/代际计数/延迟回收；不要在事件可能未处理完时复用 ds |
| I3 | `reactor.ipp:187-213` | 同一方向第二个 op 直接覆盖 `ds->read_op_/write_op_`，第一个 handler 静默丢失，`pending_operations_` 计数泄漏，`run()` 可能永不返回（asio 语义下属调用方违规，但无断言/文档） | 每方向 op 队列；或返回错误/断言且不增加计数 |
| I4 | `timer_set_base.hpp:45-54`、`basic_timer_service.hpp:108`、`signal_set_queue.hpp:48-61` | 重复 `async_wait` 会覆盖 op、把同一指针二次插入容器、索引失配；触发时调度 moved-from operation。**实测 `calls=1` + `bad_function_call`** | `async_wait` 先取消旧 op（提交 `operation_abort`）或拒绝重复 arm；插入前检查 `has_*` |
| I5 | `signal_set_service.hpp:63-84`、`signal_set_queue.hpp:92-98`、`reactor.ipp:332-343,417-427` | 信号与 `signalfd` 注册永不撤销：最后一个 signal_set 析构后信号仍被 block 并被 reactor 读掉丢弃；`sigprocmask` 只影响调用线程，worker 线程未 block 时信号可能直接终止进程 | 引用计数信号；撤销时重算 signalfd mask 并 unblock；在所有 reactor 线程 block mask |
| I6 | `socket_ops.hpp:285-340` | `poll_read/poll_write/poll_connect` 不重试 EINTR，同步收发/accept/connect 与 SSL 握手会因信号伪失败 | `while (::poll(...) == -1 && errno == EINTR)` |
| I7 | `scheduler.ipp:37-48,63-67` | `lock_guard` 与手动 `unlock()` 混用；handler 抛异常时析构会 unlock 未持有的 mutex（UB），且 `operation_decrease()` 被跳过 | 改用 `unique_lock` + 明确作用域；计数用 scope guard |
| I8 | `strand_service.hpp:52-88` | handler 抛异常时跳过 `executing_ = false`/续排逻辑，strand 永久卡死 | finally 式复位与续排 |

### 3.2 网络 / SSL

| ID | 位置 | 问题 | 建议 |
|---|---|---|---|
| I9 | `ssl/detail/error.hpp:96-97` | `SSL_ERROR_ZERO_RETURN`（正常 close_notify）映射成 `stream_truncated`，应为 `eof` | 映射为 `asio::error::eof` |
| I10 | `socket_ops.hpp:135-143` | 零长度 `recv` 被当作 eof；`is_stream_socket` 失败时其 ec 被 `success` 覆盖 | 仅在 `view.length() > 0` 时判定 EOF；传播 getsockopt 失败 |
| I11 | `socket_ops.hpp:360-363` | `AI_PASSIVE` 恒开，空 host 传 `""` 而非 `nullptr`。**实测 `getaddrinfo("",...)= -2`** | `host.empty() ? nullptr : host.c_str()` |
| I12 | `basic_unix_endpoint.hpp:36-43,61-64` | 长度检查 `>` 应为 `>=`（108 字节路径无 NUL）；`size()` 恒为 `sizeof(sockaddr_un)`，抽象命名空间地址错误 | 拒绝 `>= sizeof(sun_path)`；按实际路径长度计算 size |
| I13 | `socket_service_base.hpp:157-163,324-331` | 只把 `EINPROGRESS` 当作进行中；AF_UNIX backlog 满的 `EAGAIN` 直接上报失败 | 将 `EAGAIN/EWOULDBLOCK` 与 EINPROGRESS 同等处理 |
| I14 | `ssl/detail/stream_socket_core.hpp:170-178 等`、`ssl/detail/error.hpp:84` | OpenSSL 错误队列从不 `ERR_clear_error()`；每次操作 `SSL_get_error` 调用两次；`map_ssl_error` 用 `ERR_peek_last_error` 可能读到陈旧错误 | 每次 I/O 前 clear；只计算一次 `SSL_get_error` 并传入映射函数 |
| I15 | `ssl/detail/stream_socket_core.hpp:145-156` | `close()` 忽略 `SSL_shutdown` 返回值，flush 用 `ignored_ec`；`shutdown_failed` 枚举从未使用 | 检查返回值（含 WANT_READ/WRITE），把 flush 失败上报为 `shutdown_failed` |

### 3.3 HTTP

| ID | 位置 | 问题 | 建议 |
|---|---|---|---|
| I16 | `basic_parser.ipp:436-444` | `header_limit_` 只在不完整行分支检查；完整行可无限累加。**实测 limit=100 收下 40 条头** | 对每条完整行（含 first line、结束空行）累加并检查上限 |
| I17 | `basic_parser.ipp:542-573,614-644` | chunk-size 行与 trailer 没有任何长度上限：无 CRLF 的超长 size 行、海量 trailer 都会无限增长 | size 行/trailer 计入 header limit |
| I18 | `basic_parser.ipp:566` | `body_parsed_ + sz > body_limit_` 整数回绕。**实测 `FFFFFFFFFFFFFFFF` 被接受** | `sz > body_limit_ - body_parsed_` |
| I19 | `basic_parser.ipp:134-152` | 字段名不校验 token，值中的裸 CR/LF/NUL 不拒绝；`Host : x` 可绕过 get_field；round-trip 可 header injection | 校验 RFC 7230 token 与 CTL；拒绝 obs-fold |
| I20 | `basic_parser.ipp:634-640` | trailer 与真实 header 混在同一集合。**实测 trailer 的 `Content-Length: 999` 在完成后可见** | 单独存放 trailer；禁止 CL/TE/Host 等 framing 字段 |
| I21 | `status.hpp`、`method.hpp` | 状态码/方法为封闭枚举：103/418/425、扩展方法会被拒（客户端场景） | 接受任意 3 位状态码与任意合法 method token（或补充枚举） |

### 3.4 log4cc

| ID | 位置 | 问题 | 建议 |
|---|---|---|---|
| I22 | `sink_base.hpp:15-39` | 有多态接口但没有虚析构；`shared_ptr<sink_base>(new 派生)` 删除是 UB 且跳过派生析构（文件不 close、数据不 flush） | 加 `virtual ~sink_base() = default;` |
| I23 | `logger.hpp:44-72,88-97` | `level_`、`sinks_`、`formatter_` 全部无同步；运行时 `set_level/add_sink/set_formatter` 与 `log()` 并发即数据竞争/UB | `atomic<level>`；sinks_/formatter_ 加锁并在锁外迭代/格式化 |
| I24 | `sink_base.hpp:23-31` + `basic_*_sink.hpp` | `_mt` sink 在获取 `mutex_` 之前读取/解引用 `formatter_`，`set_formatter` 完全无锁 | 在锁内拷贝 formatter shared_ptr，锁外格式化；setter 用同一把锁 |
| I25 | `basic_async_tcp_sink.hpp:85-124,161-262` | `set_error_func/set_per_record_func` 从用户线程写、io 线程读 `std::function`；`log()` 在调用线程判空 formatter，lambda 内不判空 → 可空指针解引用 | setter 也 post 到 strand；`log()` 捕获局部 formatter 拷贝并在 lambda 内复查 |
| I26 | `basic_single_file_sink.hpp:58-76`、`basic_rotating_file_sink.hpp:62-102` | 写流不检查失败（badbit 后永久静默丢日志）；rotate 中 rename/open 抛异常后 `ofs_` 关闭、`written_` 未复位，之后每次 `log()` 都抛异常；`logger::reload()` 一个 sink 抛异常会中断其余 sink | 检查流状态并可恢复；rotate 保证异常安全；per-sink 隔离失败 |

---

## 4. Minor

| ID | 位置 | 问题 |
|---|---|---|
| M1 | `log4cc/detail/impl/items.ipp:52-67` | `%d{...}` 结果超过 64 字节时 `strftime` 返回 0，时间戳静默消失 |
| M2 | `log4cc/detail/file_ops.hpp:45-102` | `scan_file_index` 用 `std::stoul`，超长文件名抛 `out_of_range`；`"/app.log"` 时 dir 为空，返回 0 导致归档覆盖 |
| M3 | `basic_async_tcp_sink.hpp:67-78,227-262` | `reload()` 先 `++epoch_` 再 close，已部分写出的消息会整条重发（at-least-once + 截断） |
| M4 | `basic_async_tcp_sink.hpp:161-183,67-78` | 两次 `reload()` 排在 init lambda 之前时，第二个 init 会 `already_open`（文档建议的 error_func 又会 reload） |
| M5 | `logger.hpp:54-72` | `set_formatter` 只改 logger 自己的指针，不影响已 `add_sink` 的 sink；`set_formatter(nullptr)` 可能让 sink 静默丢日志 |
| M6 | `system/error.hpp:161-175` | `error_code` 隐式转 `int`，`ec == 枚举` 只比较 errnum 不比 category（asio `operation_abort`=1 会等于 system `EPERM`=1） |
| M7 | `scheduler.ipp:181-190` + `reactor.ipp:487-518` | teardown 时 `reactor_mutex_ → ds/tsq/ssq` 与 `ds/tsq/ssq → reactor_mutex_` 的理论反转（仅在“销毁时仍有线程运行”的违规用法下） |
| M8 | `object_pool.hpp:28-54` | `free` 后对象不重置内部状态，`alloc` 直接复用；对 descriptor/strand 等状态对象依赖调用方自行清空 |

---

## 5. 修复优先级建议

1. **scheduler 唤醒协议**（C1–C3）：改动最小、收益最大。补 work_guard、延迟创建服务、复用 io_context 的回归测试。
2. **reactor op 生命周期**（C4–C5 + I2 + I3）：决定多线程可靠性。可考虑引入 asio 式 `perform()`/`not_done`，同时解决 EAGAIN 语义。
3. **HTTP 分帧与限额**（C7–C8 + I16–I20）：若 parser 面向网络输入，这是安全项，应优先于功能开发。
4. **SSL 引擎**（C6 + I9 + I14 + I15）：拆读写 buffer、串行化、修正错误映射。
5. **log4cc 生命周期与同步**（C9 + I22–I26）：shared_from_this/weak、虚析构、配置加锁、文件错误处理。
6. **Minor 清理 + 工程化**：`ctest` 注册、CI（ASan/TSan）、README/线程模型/生命周期文档。

---

## 6. 已排除 / 已修复项（勿重复上报）

以下问题在本轮审查之前已修复或已作为约定接受：

- reactor 在 EPOLLERR/HUP 时预置 `bad_file_descriptor`（已修复：不再预置，由 syscall 产生真实 errno）。
- scheduler `wake_up_one_thread` / `stop_all_threads` 的 `op_queue_mutex_ ↔ reactor_mutex_` 锁序反转（已修复，TSan 验证通过）。
- HTTP `stream_read` 先读 socket 后解析导致已缓冲数据不被消费（已修复：parse-first）。
- strand 在仍有排队 handler 时析构：约定由调用方保证生命周期（与 asio 行为不同，建议文档化）。
- `async_tcp_sink` 要求用户在 `error_func_` 中调用 `reload()`：设计选择。
- 缺少 asio 的 `perform()/not_done` 语义，spurious readiness 的 EAGAIN 可能直达 handler：已知架构差距（与 I3/C4 一并考虑）。
- HTTP async 接口在数据已缓冲时可能内联回调：已知，尚未决定处理方式。
- 文档/CI/test 注册缺失：已在总结中单独列出。

---

## 7. 验证记录

### 7.1 实测复现

| 项 | 方法 | 结果 |
|---|---|---|
| C2 首次 reactor 不唤醒 | `io_context(2)` + work_guard，先起线程，后建 timer | `fired before any post = 0`；`post` 后 `fired = 1` |
| C3 单线程永久睡眠 | `io_context(1)` + work_guard，run 在另一线程 | stop 前后 `run returned = 0` |
| I1 永久 stop | `post(a); run(); post(b); run();` | `a=1 b=0` |
| I4 timer 重复 arm | 同一 timer 连续两次 `expires_after + async_wait` | `calls=1`，抛 `std::bad_function_call` |
| C5 投机 op 绕过 cancel | `async_write_some` 后立即 `close()` | handler `ec='Bad file descriptor'`（应为 `operation_aborted`） |
| C7 HTTP 分帧 | 冲突 CL / TE+CL / 多 TE 行 | 冲突 CL 不报错；`TE: gzip` + `TE: chunked` + `CL:5` 余 10 字节成为下一请求 |
| C8 304 分帧 | `304` + `CL: 10` + 后续响应 | body 被填成 `GET / HTTP` |
| I16 header_limit | `header_limit(100)` + 40 条完整头 | `ec=Wait more data`，40 字段全部接受 |
| I17/I18 chunk | 无 CRLF 的 size 行 / `FFFFFFFFFFFFFFFF` | 无上限；`body_limit=10` 被绕过 |
| I20 trailer 注入 | trailer 中放 `Content-Length: 999` | 完成后 `get_field` 返回 999 |
| I11 空 host 解析 | `getaddrinfo("", "8080", AI_PASSIVE)` | 返回 `-2 EAI_NONAME` |
| 锁序（已修） | TSan 检查（旧代码 / 新代码） | 旧代码报 `lock-order-inversion`；新代码零告警 |

### 7.2 仅代码确认（未做动态复现）

- C1 丢失唤醒的窗口（reviewer 另有复现记录）。
- C6 SSL 共享 buffer、I9/I14/I15 的 SSL 错误路径（需要真实 TLS 握手压测）。
- I2 descriptor 状态池化复用。
- I5 信号撤销、I6 EINTR、I12/I13 的 socket 边缘行为。
- I22–I26 的 log4cc 生命周期与竞争问题。

### 7.3 工具说明

- TSan 能覆盖锁序/数据竞争，但对本轮多数“逻辑竞态”（唤醒协议、op 槽位覆盖、ds 复用）无效；这些需要针对性压力测试或模型检查。
- 未做性能基准；`std::function` operation 的分配/拷贝开销未量化。

---

## 8. 审查范围说明

- 本次为静态审查 + 定点动态复现，不是完整的形式化验证。
- 未覆盖：真实 TLS 握手/大量并发下的 SSL 行为、resolver 全面行为、长期运行的文件轮转、跨平台（库本身是 epoll-only）。
- 测试代码不在本报告范围内，仅在需要理解行为时参考。
