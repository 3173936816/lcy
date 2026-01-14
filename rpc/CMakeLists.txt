add_library(lcy_rpc SHARED
	src/details/connection.cc
	src/details/server.cc

 	src/rpc.pb.cc
	src/server.cc
	src/channel.cc
)

target_include_directories(lcy_rpc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}  # 添加这一行，以便能找到 asio 的头文件
)

target_link_libraries(lcy_rpc PUBLIC lcy_asio protobuf)

# 添加测试子目录
add_subdirectory(tests)

# ========== 添加安装选项 ==========

# 安装头文件到系统目录
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/lcy/rpc
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

install(FILES rpc.hpp
    DESTINATION include/lcy/rpc
)

# 安装库文件
install(TARGETS lcy_rpc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin  # Windows 系统需要
)
