# 启用测试
enable_testing()

# 测试可执行文件

add_executable(test_http_request test_http_request.cc)
target_link_libraries(test_http_request lcy_protocol pthread)
add_test(NAME test_http_request COMMAND test_http_request)

add_executable(test_http_response test_http_response.cc)
target_link_libraries(test_http_response lcy_protocol pthread)
add_test(NAME test_http_response COMMAND test_http_response)


# 设置输出目录
set_target_properties(
	test_http_request
	test_http_response
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
