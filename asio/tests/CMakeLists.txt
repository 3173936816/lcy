# 启用测试
enable_testing()

# 测试可执行文件
add_executable(test_bridge_service test_bridge_service.cc)
target_link_libraries(test_bridge_service lcy_asio pthread)
add_test(NAME test_bridge_service COMMAND test_bridge_service)

add_executable(test_io_context test_io_context.cc)
target_link_libraries(test_io_context lcy_asio pthread)
add_test(NAME test_io_context COMMAND test_io_context)

add_executable(test_timer_service test_timer_service.cc)
target_link_libraries(test_timer_service lcy_asio pthread)
add_test(NAME test_timer_service COMMAND test_timer_service)

add_executable(test_signal_set test_signal_set.cc)
target_link_libraries(test_signal_set lcy_asio pthread)
add_test(NAME test_signal_set COMMAND test_signal_set)

add_executable(test_steady_timer test_steady_timer.cc)
target_link_libraries(test_steady_timer lcy_asio pthread)
add_test(NAME test_steady_timer COMMAND test_steady_timer)

add_executable(test_tcp_socket test_tcp_socket.cc)
target_link_libraries(test_tcp_socket lcy_asio pthread)
add_test(NAME test_tcp_socket COMMAND test_tcp_socket)

add_executable(test_udp_socket test_udp_socket.cc)
target_link_libraries(test_udp_socket lcy_asio pthread)
add_test(NAME test_udp_socket COMMAND test_udp_socket)

add_executable(test_acceptor test_acceptor.cc)
target_link_libraries(test_acceptor lcy_asio pthread)
add_test(NAME test_acceptor COMMAND test_acceptor)

# 设置输出目录
set_target_properties(
    test_bridge_service
    test_io_context
    test_timer_service
    test_signal_set
    test_steady_timer
    test_tcp_socket
    test_udp_socket
    test_acceptor
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
