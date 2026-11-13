####### lcy

add_executable(object_pool_test object_pool.cc)
target_include_directories(object_pool_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

####### system

add_executable(error_test system/error.cc)
target_include_directories(error_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

####### asio

add_executable(timer_set_queue_test asio/timer_set_queue.cc)
target_include_directories(timer_set_queue_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(signal_set_queue_test asio/signal_set_queue.cc)
target_include_directories(signal_set_queue_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(signal_set_test asio/signal_set.cc)
target_include_directories(signal_set_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(signal_set_service_test asio/signal_set_service.cc)
target_include_directories(signal_set_service_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(steady_timer_test asio/steady_timer.cc)
target_include_directories(steady_timer_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(stream_socket_service_test asio/stream_socket_service.cc)
target_include_directories(stream_socket_service_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(scheduler_test asio/scheduler.cc)
target_include_directories(scheduler_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(basic_timer_service_test asio/basic_timer_service.cc)
target_include_directories(basic_timer_service_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(udp_socket_test asio/udp_socket.cc)
target_include_directories(udp_socket_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(tcp_socket_test asio/tcp_socket.cc)
target_include_directories(tcp_socket_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(basic_resolver_test asio/basic_resolver.cc)
target_include_directories(basic_resolver_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(strand_test asio/strand.cc)
target_include_directories(strand_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(work_guard_test asio/work_guard.cc)
target_include_directories(work_guard_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(async_stream_test asio/async_stream.cc)
target_include_directories(async_stream_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(func_type_test asio/func_type.cc)
target_include_directories(func_type_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

####### asio ssl

add_executable(ssl_stream_socket asio/ssl/stream_socket.cc)
target_include_directories(ssl_stream_socket PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ssl_stream_socket PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

add_executable(ssl_c_client asio/ssl/ssl_c_client.cc)
target_include_directories(ssl_c_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ssl_c_client PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

####### bytra

add_executable(flat_buffer_test bytra/flat_buffer.cc)
target_include_directories(flat_buffer_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(ostream_test bytra/ostream.cc)
target_include_directories(ostream_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(header_fields_test bytra/http/header_fields.cc)
target_include_directories(header_fields_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(http_request_test bytra/http/request.cc)
target_include_directories(http_request_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(http_response_test bytra/http/response.cc)
target_include_directories(http_response_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(http_parser_test bytra/http/parser.cc)
target_include_directories(http_parser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(http_async_stream_test bytra/http/async_stream.cc)
target_include_directories(http_async_stream_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

####### server

add_executable(http_server_test server/http_server.cc)
target_include_directories(http_server_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(ssl_http_server_test server/ssl_http_server.cc)
target_include_directories(ssl_http_server_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(ssl_http_server_test PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

####### epoll

add_executable(epoll_behavior_test epoll/epoll_behavior.cc)
target_include_directories(epoll_behavior_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)


###### log4cc
add_executable(log4cc_formatter_test log4cc/formatter.cc)
target_include_directories(log4cc_formatter_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)