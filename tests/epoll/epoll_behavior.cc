#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <errno.h>

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

const char* event_str(uint32_t ev, char* buf, size_t) {
    buf[0] = '\0';
    if (ev & EPOLLIN)  strcat(buf, "EPOLLIN ");
    if (ev & EPOLLOUT) strcat(buf, "EPOLLOUT ");
    if (ev & EPOLLERR) strcat(buf, "EPOLLERR ");
    if (ev & EPOLLHUP) strcat(buf, "EPOLLHUP ");
    if (ev & EPOLLRDHUP) strcat(buf, "EPOLLRDHUP ");
    return buf;
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9999);
    bind(listen_fd, (sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 5);
    printf("[server] 监听 127.0.0.1:9999\n");

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    connect(client_fd, (sockaddr*)&server, sizeof(server));
    printf("[client] 已连接\n");

    // 3. accept
    int conn_fd = accept(listen_fd, nullptr, nullptr);
    set_nonblocking(conn_fd);
    printf("[server] accept, conn_fd=%d\n", conn_fd);

    int epfd = epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = conn_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);
    printf("[server] 注册 EPOLLIN | EPOLLOUT | EPOLLET\n");

    pid_t pid = fork();
    if (pid == 0) {
        sleep(1);
        printf("[client] 发送 AAAA\n");
        send(client_fd, "AAAA", 4, 0);
        sleep(2);
        printf("[client] 发送 BBBB\n");
        send(client_fd, "BBBB", 4, 0);
        sleep(3);
        close(client_fd);
        _exit(0);
    }

    char buf[1024];
    int round = 0;
    while (round < 20) {
        epoll_event events[10];
        int n = epoll_wait(epfd, events, 10, 3000);
        if (n < 0) { perror("epoll_wait"); break; }
        if (n == 0) {
            printf("[server] epoll_wait 超时\n");
            break;
        }

        for (int i = 0; i < n; ++i) {
            char evbuf[128];
            printf("[server] 第 %d 次返回: fd=%d events=0x%x (%s)\n",
                   round, events[i].data.fd, events[i].events,
                   event_str(events[i].events, evbuf, sizeof(evbuf)));

            if (events[i].events & EPOLLIN) {
                ssize_t r = read(conn_fd, buf, 1);
                if (r > 0) {
                    printf("[server] 读了 1 字节: '%c'\n", buf[0]);
                } else if (r == 0) {
                    printf("[server] 对端关闭\n");
                } else {
                    printf("[server] read -1, errno=%d (%s)\n", errno, strerror(errno));
                }
            }

            if (events[i].events & EPOLLOUT) {
                printf("[server] EPOLLOUT 触发（可写，但本示例不写数据）\n");
            }
        }
        round++;
    }

    close(conn_fd);
    close(client_fd);
    close(listen_fd);
    close(epfd);
    return 0;
}