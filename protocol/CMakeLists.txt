## 创建 protocol 库
#add_library(lcy_protocol SHARED
#    src/http/message.cc
#    src/http/message.h
#    src/http/request.cc
#    src/http/request.h
#    src/http/response.cc
#    src/http/response.h
#    src/http/parser.cc
#    src/http/parser.h
#    src/http/servlet.cc
#    src/http/servlet.h
#)

# 注意：.h 文件不应该添加到 add_library 中，它们是头文件
# 应该只添加源文件（.cc 文件）
# 修正后的版本：
add_library(lcy_protocol SHARED
    src/http/message.cc
    src/http/request.cc
    src/http/response.cc
    src/http/parser.cc
    src/http/servlet.cc

	src/tlv/variant_encode.cc
	src/tlv/message.cc
	src/tlv/parser.cc
)

# 设置目标包含目录 - 这里有个错误，应该是 lcy_protocol，不是 lcy_asio
target_include_directories(lcy_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}  # 添加这一行，以便能找到 asio 的头文件
)

# 链接库 - protocol 库需要链接 asio 库，而不是 Threads
# 首先需要找到 asio 库（如果 asio 在同一项目中）
# 然后在链接时添加：
target_link_libraries(lcy_protocol PUBLIC lcy_asio)

# Threads 库通常由 asio 库链接，protocol 库不需要直接链接

# 添加测试子目录
add_subdirectory(tests)

# ========== 添加安装选项 ==========

# 安装头文件到系统目录
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/lcy/protocol
    FILES_MATCHING 
    PATTERN "*.h" 
    PATTERN "*.hpp"
    PATTERN "tests" EXCLUDE
    PATTERN "CMakeLists.txt" EXCLUDE
    PATTERN "*.cc" EXCLUDE
    PATTERN "CMakeFiles" EXCLUDE
    PATTERN "bin" EXCLUDE
    PATTERN "lib" EXCLUDE
)

# 安装 protocol.hpp 到顶层目录
install(FILES protocol.hpp
    DESTINATION include/lcy/protocol
)

# 安装库文件
install(TARGETS lcy_protocol
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin  # Windows 系统需要
)
