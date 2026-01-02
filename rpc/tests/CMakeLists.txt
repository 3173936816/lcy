# 启用测试
enable_testing()

# 测试可执行文件
#add_executable(test_client_session test_client_session.cc)
#target_link_libraries(test_client_session lcy_asio pthread lcy_rpc)
#add_test(NAME test_client_session COMMAND test_client_session)

# 设置输出目录
set_target_properties(
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
