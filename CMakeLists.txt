cmake_minimum_required(VERSION 3.27.4)
project(lcy LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 11)
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-g -Wall -Wextra -Werror -Wno-implicit-fallthrough)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#######	lcy

add_executable(object_pool_test tests/object_pool.cc)
target_include_directories(object_pool_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

####### system

add_executable(error_test tests/system/error.cc)
target_include_directories(error_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

####### asio

add_executable(timer_set_queue_test tests/asio/timer_set_queue.cc)
target_include_directories(timer_set_queue_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(signal_set_queue_test tests/asio/signal_set_queue.cc)
target_include_directories(signal_set_queue_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(signal_set_test tests/asio/signal_set.cc)
target_include_directories(signal_set_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(signal_set_service_test tests/asio/signal_set_service.cc)
target_include_directories(signal_set_service_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(steady_timer_test tests/asio/steady_timer.cc)
target_include_directories(steady_timer_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(stream_socket_service_test tests/asio/stream_socket_service.cc)
target_include_directories(stream_socket_service_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(scheduler_test tests/asio/scheduler.cc)
target_include_directories(scheduler_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(basic_timer_service_test tests/asio/basic_timer_service.cc)
target_include_directories(basic_timer_service_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(udp_socket_test tests/asio/udp_socket.cc)
target_include_directories(udp_socket_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tcp_socket_test tests/asio/tcp_socket.cc)
target_include_directories(tcp_socket_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(basic_resolver_test tests/asio/basic_resolver.cc)
target_include_directories(basic_resolver_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(strand_test tests/asio/strand.cc)
target_include_directories(strand_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(work_guard_test tests/asio/work_guard.cc)
target_include_directories(work_guard_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(async_stream_test tests/asio/async_stream.cc)
target_include_directories(async_stream_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


####### bytra

add_executable(http_request_test tests/bytra/http/request.cc)
target_include_directories(http_request_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
