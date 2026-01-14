# 启用测试
enable_testing()

# 测试可执行文件

add_executable(test_http_request test_http_request.cc)
target_link_libraries(test_http_request lcy_protocol pthread)
add_test(NAME test_http_request COMMAND test_http_request)

add_executable(test_http_response test_http_response.cc)
target_link_libraries(test_http_response lcy_protocol pthread)
add_test(NAME test_http_response COMMAND test_http_response)

add_executable(test_http_parser test_http_parser.cc)
target_link_libraries(test_http_parser lcy_protocol pthread)
add_test(NAME test_http_parser COMMAND test_http_parser)

add_executable(test_tlv_variant_encode test_tlv_variant_encode.cc)
target_link_libraries(test_tlv_variant_encode lcy_protocol pthread)
add_test(NAME test_tlv_variant_encode COMMAND test_tlv_variant_encode)

add_executable(test_tlv_message test_tlv_message.cc)
target_link_libraries(test_tlv_message lcy_protocol pthread)
add_test(NAME test_tlv_message COMMAND test_tlv_message)

add_executable(test_tlv_parser test_tlv_parser.cc)
target_link_libraries(test_tlv_parser lcy_protocol pthread)
add_test(NAME test_tlv_parser COMMAND test_tlv_parser)

# 设置输出目录
set_target_properties(
	test_http_request
	test_http_response
	test_http_parser
	
	test_tlv_variant_encode
	test_tlv_message
	test_tlv_parser

    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)
