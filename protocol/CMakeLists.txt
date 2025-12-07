# 创建 protocol 库
add_library(lcy_protocol SHARED
	src/http/message.cc
	src/http/message.h
	src/http/request.cc
	src/http/request.h
	src/http/response.cc
	src/http/response.h
	src/http/servlet.cc
	src/http/servlet.h
)

# 如果有 .ipp 文件，确保它们被正确包含
# 这些文件通过 #include 在 .hpp 文件中被包含，不需要添加到源文件列表

# 设置目标包含目录
target_include_directories(lcy_asio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接库
find_package(Threads REQUIRED)
target_link_libraries(lcy_protocol PUBLIC Threads::Threads)

# 添加测试子目录
add_subdirectory(tests)
