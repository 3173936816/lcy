#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
大型chunked HTTP请求客户端模拟器
支持流式发送大数据，模拟真实场景的chunked传输
"""

import socket
import ssl
import time
import json
import random
import threading
import argparse
from datetime import datetime
from typing import List, Dict, Any, Optional

class ChunkedHttpClient:
    """支持chunked传输的大型HTTP请求客户端"""
    
    def __init__(self, host: str, port: int = 80, use_ssl: bool = False, 
                 timeout: int = 30, verbose: bool = True):
        """
        初始化客户端
        
        Args:
            host: 目标主机
            port: 目标端口
            use_ssl: 是否使用HTTPS
            timeout: 超时时间（秒）
            verbose: 是否显示详细日志
        """
        self.host = host
        self.port = port
        self.use_ssl = use_ssl
        self.timeout = timeout
        self.verbose = verbose
        self.socket = None
        
        # 统计信息
        self.stats = {
            'total_chunks': 0,
            'total_bytes': 0,
            'start_time': None,
            'end_time': None,
            'transfer_rate': 0
        }
    
    def connect(self) -> None:
        """建立连接"""
        try:
            # 创建socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(self.timeout)
            
            # 连接
            if self.verbose:
                print(f"[{datetime.now()}] 连接到 {self.host}:{self.port}...")
            
            self.socket.connect((self.host, self.port))
            
            # 如果需要SSL/TLS
            if self.use_ssl:
                context = ssl.create_default_context()
                self.socket = context.wrap_socket(self.socket, server_hostname=self.host)
            
            if self.verbose:
                print(f"[{datetime.now()}] 连接成功")
                
        except Exception as e:
            raise ConnectionError(f"连接失败: {e}")
    
    def disconnect(self) -> None:
        """断开连接"""
        if self.socket:
            self.socket.close()
            self.socket = None
            if self.verbose:
                print(f"[{datetime.now()}] 连接已关闭")
    
    def generate_large_payload(self, target_size_mb: float = 10.0) -> List[str]:
        """
        生成大型测试负载
        
        Args:
            target_size_mb: 目标大小（MB）
            
        Returns:
            分块后的字符串列表
        """
        if self.verbose:
            print(f"[{datetime.now()}] 生成 {target_size_mb}MB 测试数据...")
        
        target_bytes = int(target_size_mb * 1024 * 1024)
        chunks = []
        current_size = 0
        
        # 基础JSON结构
        base_structure = {
            "request_id": f"req_{random.randint(100000, 999999)}",
            "timestamp": datetime.now().isoformat(),
            "client_info": {
                "name": "ChunkedStressClient",
                "version": "2.1.0",
                "platform": "Python/3.9"
            },
            "data_type": "performance_test",
            "chunks": []
        }
        
        chunk_id = 0
        chunk_size_variation = [1024, 2048, 4096, 8192, 16384]  # 不同大小的chunk
        
        while current_size < target_bytes:
            # 随机选择chunk大小
            chunk_size = random.choice(chunk_size_variation)
            
            # 生成一个chunk的数据
            chunk_data = {
                "chunk_id": chunk_id,
                "sequence": chunk_id,
                "size": chunk_size,
                "timestamp": datetime.now().isoformat(),
                "random_data": ''.join(random.choices('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789', k=min(500, chunk_size))),
                "metrics": {
                    "cpu_load": random.uniform(0.1, 0.9),
                    "memory_used": random.randint(100, 1000),
                    "network_speed": random.randint(1, 1000),
                    "latency": random.uniform(0.1, 10.0)
                },
                "sample_values": [random.randint(1, 1000) for _ in range(10)],
                "binary_sample": "".join(format(random.randint(0, 255), '08b') for _ in range(50))
            }
            
            # 序列化为JSON字符串
            chunk_json = json.dumps(chunk_data, separators=(',', ':'))
            
            # 如果需要填充以接近目标chunk大小
            if len(chunk_json) < chunk_size:
                padding = chunk_size - len(chunk_json)
                chunk_json = chunk_json[:-1] + f',"padding":"{"x" * padding}"' + "}"
            
            chunks.append(chunk_json)
            current_size += len(chunk_json)
            chunk_id += 1
            
            # 更新基础结构中的chunks列表（只保留元数据）
            if len(base_structure["chunks"]) < 100:  # 避免太大
                base_structure["chunks"].append({
                    "chunk_id": chunk_id,
                    "size": len(chunk_json)
                })
        
        # 添加头部和尾部chunk
        header_chunk = json.dumps(base_structure)
        footer_chunk = json.dumps({
            "end_of_transmission": True,
            "total_chunks": len(chunks),
            "total_bytes": current_size,
            "final_timestamp": datetime.now().isoformat(),
            "hash": f"sha256_{random.getrandbits(256):064x}"
        })
        
        # 组合所有chunks
        all_chunks = [header_chunk] + chunks + [footer_chunk]
        
        # 计算实际大小
        actual_mb = current_size / (1024 * 1024)
        if self.verbose:
            print(f"[{datetime.now()}] 数据生成完成: {len(all_chunks)} chunks, "
                  f"{actual_mb:.2f}MB ({current_size:,} 字节)")
        
        return all_chunks
    
    def send_request(self, path: str = "/upload/stream", 
                    chunks: Optional[List[str]] = None,
                    chunk_delay: float = 0.01,
                    target_size_mb: float = 5.0) -> str:
        """
        发送chunked HTTP请求
        
        Args:
            path: 请求路径
            chunks: 预先生成的chunks（如果为None则自动生成）
            chunk_delay: 每个chunk之间的延迟（秒）
            target_size_mb: 目标数据大小（MB）
            
        Returns:
            服务器响应
        """
        # 记录开始时间
        self.stats['start_time'] = datetime.now()
        
        try:
            # 建立连接
            self.connect()
            
            # 如果没有提供chunks，则生成
            if chunks is None:
                chunks = self.generate_large_payload(target_size_mb)
            
            self.stats['total_chunks'] = len(chunks)
            self.stats['total_bytes'] = sum(len(chunk.encode('utf-8')) for chunk in chunks)
            
            # 构建请求头
            headers = [
                f"POST {path} HTTP/1.1",
                f"Host: {self.host}",
                "User-Agent: LargeChunkedClient/3.0 (Python)",
                "Content-Type: application/json",
                "Transfer-Encoding: chunked",
                "Accept: application/json",
                "X-Request-Type: large_stream_upload",
                f"X-Expected-Size: {self.stats['total_bytes']}",
                f"X-Chunk-Count: {len(chunks)}",
                "X-Compression: none",
                "Connection: keep-alive",
                "X-Timestamp: " + datetime.now().isoformat(),
                "\r\n"
            ]
            
            # 发送请求头
            request_headers = "\r\n".join(headers)
            self.socket.sendall(request_headers.encode('utf-8'))
            
            if self.verbose:
                print(f"[{datetime.now()}] 开始发送 {len(chunks)} 个chunks...")
                print(f"[{datetime.now()}] 请求头已发送")
            
            # 发送chunked数据
            progress_interval = max(1, len(chunks) // 20)  # 每5%显示一次进度
            
            for i, chunk in enumerate(chunks):
                # 转换为字节并获取大小
                chunk_bytes = chunk.encode('utf-8')
                chunk_size = len(chunk_bytes)
                
                # 发送chunk大小（十六进制）
                size_line = f"{chunk_size:X}\r\n"
                self.socket.sendall(size_line.encode('utf-8'))
                
                # 发送chunk数据
                self.socket.sendall(chunk_bytes)
                self.socket.sendall(b"\r\n")
                
                # 显示进度
                if self.verbose and i % progress_interval == 0:
                    percent = (i + 1) / len(chunks) * 100
                    print(f"[{datetime.now()}] 进度: {percent:.1f}% "
                          f"({i+1}/{len(chunks)} chunks, "
                          f"{sum(len(c.encode('utf-8')) for c in chunks[:i+1]):,} 字节)")
                
                # 延迟以模拟流式传输
                if chunk_delay > 0:
                    time.sleep(chunk_delay)
            
            # 发送结束标记
            self.socket.sendall(b"0\r\n\r\n")
            
            if self.verbose:
                print(f"[{datetime.now()}] 所有数据已发送")
                print(f"[{datetime.now()}] 发送结束标记")
            
            # 接收响应
            response = self._receive_response()
            
            # 记录结束时间并计算统计
            self.stats['end_time'] = datetime.now()
            self._calculate_stats()
            
            return response
            
        except socket.timeout:
            raise TimeoutError("请求超时")
        except Exception as e:
            raise RuntimeError(f"发送请求失败: {e}")
        finally:
            self.disconnect()
    
    def _receive_response(self) -> str:
        """接收服务器响应"""
        if self.verbose:
            print(f"[{datetime.now()}] 等待服务器响应...")
        
        response_data = b""
        buffer_size = 4096
        
        try:
            # 设置接收超时
            self.socket.settimeout(self.timeout)
            
            while True:
                try:
                    chunk = self.socket.recv(buffer_size)
                    if not chunk:
                        break
                    response_data += chunk
                    
                    # 检查是否收到完整的HTTP响应
                    if b"\r\n\r\n" in response_data:
                        # 尝试解析内容长度
                        header_end = response_data.find(b"\r\n\r\n")
                        headers = response_data[:header_end].decode('utf-8', errors='ignore')
                        
                        # 检查是否为chunked响应
                        if "Transfer-Encoding: chunked" in headers:
                            # 继续接收直到收到结束标记
                            if response_data.endswith(b"0\r\n\r\n"):
                                break
                        else:
                            # 检查Content-Length
                            content_length = 0
                            for line in headers.split('\r\n'):
                                if line.lower().startswith('content-length:'):
                                    content_length = int(line.split(':')[1].strip())
                                    break
                            
                            if content_length > 0:
                                body_start = header_end + 4
                                if len(response_data) >= body_start + content_length:
                                    break
        
                except socket.timeout:
                    if response_data:
                        break  # 已经收到一些数据
                    else:
                        raise
        
        except Exception as e:
            if self.verbose:
                print(f"[{datetime.now()}] 接收响应时出错: {e}")
        
        return response_data.decode('utf-8', errors='ignore')
    
    def _calculate_stats(self) -> None:
        """计算传输统计信息"""
        if self.stats['start_time'] and self.stats['end_time']:
            duration = (self.stats['end_time'] - self.stats['start_time']).total_seconds()
            if duration > 0:
                self.stats['transfer_rate'] = self.stats['total_bytes'] / duration
                self.stats['duration'] = duration
    
    def print_stats(self) -> None:
        """打印传输统计信息"""
        if self.stats['start_time']:
            print("\n" + "="*60)
            print("传输统计信息:")
            print("="*60)
            print(f"开始时间:     {self.stats['start_time']}")
            print(f"结束时间:     {self.stats['end_time']}")
            print(f"持续时间:     {self.stats.get('duration', 0):.2f} 秒")
            print(f"总chunks数:   {self.stats['total_chunks']:,}")
            print(f"总字节数:     {self.stats['total_bytes']:,}")
            print(f"平均chunk大小: {self.stats['total_bytes'] / self.stats['total_chunks']:,.0f} 字节")
            
            if self.stats.get('transfer_rate', 0) > 0:
                rate_mbps = (self.stats['transfer_rate'] * 8) / (1024 * 1024)
                print(f"传输速率:     {self.stats['transfer_rate'] / 1024:.2f} KB/s ({rate_mbps:.2f} Mbps)")
            
            if self.stats['total_bytes'] > 0:
                mb_size = self.stats['total_bytes'] / (1024 * 1024)
                print(f"数据大小:     {mb_size:.2f} MB")
            print("="*60)

def benchmark_test(host: str, port: int, size_mb: float = 1.0, 
                  concurrent: int = 1, use_ssl: bool = False):
    """执行基准测试"""
    print(f"\n开始基准测试: {concurrent} 个并发请求，每个 {size_mb}MB")
    
    results = []
    threads = []
    
    def worker(worker_id: int):
        """工作线程函数"""
        try:
            client = ChunkedHttpClient(host, port, use_ssl=use_ssl, verbose=False)
            print(f"[Worker {worker_id}] 开始请求...")
            
            start_time = datetime.now()
            response = client.send_request(
                path="/upload/benchmark",
                target_size_mb=size_mb,
                chunk_delay=0.001  # 快速发送
            )
            end_time = datetime.now()
            
            duration = (end_time - start_time).total_seconds()
            client._calculate_stats()
            
            results.append({
                'worker_id': worker_id,
                'duration': duration,
                'bytes': client.stats['total_bytes'],
                'chunks': client.stats['total_chunks']
            })
            
            print(f"[Worker {worker_id}] 完成: {duration:.2f}秒")
            
        except Exception as e:
            print(f"[Worker {worker_id}] 失败: {e}")
            results.append({
                'worker_id': worker_id,
                'error': str(e)
            })
    
    # 创建并启动线程
    for i in range(concurrent):
        thread = threading.Thread(target=worker, args=(i+1,))
        threads.append(thread)
        thread.start()
    
    # 等待所有线程完成
    for thread in threads:
        thread.join()
    
    # 打印汇总结果
    if results and not all('error' in r for r in results):
        successful = [r for r in results if 'error' not in r]
        if successful:
            total_bytes = sum(r['bytes'] for r in successful)
            total_duration = max(r['duration'] for r in successful)
            avg_duration = sum(r['duration'] for r in successful) / len(successful)
            
            print(f"\n基准测试结果:")
            print(f"成功请求: {len(successful)}/{concurrent}")
            print(f"总数据量: {total_bytes / (1024*1024):.2f} MB")
            print(f"总时间:   {total_duration:.2f} 秒")
            print(f"平均时间: {avg_duration:.2f} 秒")
            print(f"总吞吐量: {(total_bytes * 8) / (total_duration * 1024 * 1024):.2f} Mbps")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description="大型chunked HTTP请求客户端",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s example.com 80 -s 10        # 发送10MB数据
  %(prog)s localhost 8080 -s 50 -d 0.5 # 发送50MB数据，chunk间隔0.5秒
  %(prog)s api.example.com 443 --ssl -s 100  # SSL加密发送100MB
  %(prog)s test.com 80 -b -c 5 -s 2    # 5并发，每个2MB基准测试
        """
    )
    
    parser.add_argument("host", help="目标主机名或IP地址")
    parser.add_argument("port", type=int, help="目标端口")
    parser.add_argument("-s", "--size", type=float, default=5.0, 
                       help="数据大小(MB，默认: 5.0)")
    parser.add_argument("-d", "--delay", type=float, default=0.01,
                       help="chunk间延迟(秒，默认: 0.01)")
    parser.add_argument("-p", "--path", default="/upload/stream",
                       help="请求路径(默认: /upload/stream)")
    parser.add_argument("--ssl", action="store_true",
                       help="使用SSL/TLS(HTTPS)")
    parser.add_argument("-t", "--timeout", type=int, default=60,
                       help="超时时间(秒，默认: 60)")
    parser.add_argument("-q", "--quiet", action="store_true",
                       help="安静模式，不显示详细日志")
    parser.add_argument("-b", "--benchmark", action="store_true",
                       help="运行基准测试模式")
    parser.add_argument("-c", "--concurrent", type=int, default=1,
                       help="并发连接数(基准测试模式，默认: 1)")
    
    args = parser.parse_args()
    
    try:
        if args.benchmark:
            # 基准测试模式
            benchmark_test(
                host=args.host,
                port=args.port,
                size_mb=args.size,
                concurrent=args.concurrent,
                use_ssl=args.ssl
            )
        else:
            # 单次请求模式
            print("="*60)
            print(f"大型chunked HTTP请求客户端")
            print(f"目标: {args.host}:{args.port}")
            print(f"大小: {args.size} MB")
            print(f"延迟: {args.delay} 秒/chunk")
            print(f"SSL:  {'是' if args.ssl else '否'}")
            print("="*60)
            
            client = ChunkedHttpClient(
                host=args.host,
                port=args.port,
                use_ssl=args.ssl,
                timeout=args.timeout,
                verbose=not args.quiet
            )
            
            response = client.send_request(
                path=args.path,
                target_size_mb=args.size,
                chunk_delay=args.delay
            )
            
            # 显示响应摘要
            if not args.quiet and response:
                print(f"\n服务器响应摘要:")
                print("-"*40)
                
                # 提取状态行
                lines = response.split('\r\n')
                if lines:
                    print(f"状态: {lines[0]}")
                
                # 显示前几行头部
                header_count = 0
                for i, line in enumerate(lines[1:], 1):
                    if not line.strip():
                        break
                    if header_count < 10:  # 显示前10个头部
                        print(f"头部: {line}")
                        header_count += 1
                
                # 显示响应体前500字符
                body_start = response.find('\r\n\r\n')
                if body_start != -1:
                    body = response[body_start + 4:]
                    if body:
                        print(f"\n响应体(前500字符):")
                        print("-"*40)
                        print(body[:500])
                        if len(body) > 500:
                            print(f"... (共{len(body)}字符)")
            
            client.print_stats()
            
    except KeyboardInterrupt:
        print("\n用户中断操作")
    except Exception as e:
        print(f"\n错误: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())
