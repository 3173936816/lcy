# 创建 asio 库
add_library(lcy_asio SHARED
	src/errinfo.h
	src/errinfo.cc
    src/io_context.cc
    src/io_context.hpp
    src/exception.cc
    src/exception.h
	src/steady_timer.h
	src/steady_timer.cc
	src/signal_set.h
	src/signal_set.cc
	src/buffer.h
	src/buffer.cc
	src/dynamic_buffer.h
	src/dynamic_buffer.cc

	src/ip/tcp.h
    src/ip/udp.h
    src/ip/endpoint.h
    src/ip/endpoint.cc

    
    src/details/bridge_service.cc
    src/details/bridge_service.hpp
    src/details/channel.cc
    src/details/channel.h
    src/details/reactor_service.cc
    src/details/reactor_service.h
    src/details/timer_service.cc
    src/details/timer_service.h
    src/details/service.hpp
	
    src/ip/details/endpoint_data.h
    src/ip/details/tcp_socket.h
    src/ip/details/tcp_socket.cc
    src/ip/details/udp_socket.h
    src/ip/details/udp_socket.cc
    src/ip/details/acceptor.h
    src/ip/details/acceptor.cc
)

# 如果有 .ipp 文件，确保它们被正确包含
# 这些文件通过 #include 在 .hpp 文件中被包含，不需要添加到源文件列表

# 设置目标包含目录
target_include_directories(lcy_asio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接库
find_package(Threads REQUIRED)
target_link_libraries(lcy_asio PUBLIC Threads::Threads)

# 添加测试子目录
add_subdirectory(tests)
