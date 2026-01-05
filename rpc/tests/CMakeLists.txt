# 启用测试
enable_testing()

# 测试可执行文件
add_executable(test_details_server test_details_server.cc)
target_link_libraries(test_details_server lcy_asio pthread lcy_rpc)
add_test(NAME test_details_server COMMAND test_details_server)

add_executable(test_details_connection test_details_connection.cc)
target_link_libraries(test_details_connection lcy_asio pthread lcy_rpc)
add_test(NAME test_details_connection COMMAND test_details_connection)

# 设置输出目录
set_target_properties(
	test_details_server
	test_details_connection

    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
