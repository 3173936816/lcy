cmake_minimum_required(VERSION 3.27.4)
project(lcy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-g -Wall -Wextra -Werror -Wno-implicit-fallthrough -Wno-unused-function)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(lcy INTERFACE)
add_library(lcy::lcy ALIAS lcy)

target_include_directories(lcy INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(lcy INTERFACE cxx_std_11)

add_library(lcy_ssl INTERFACE)
add_library(lcy::ssl ALIAS lcy_ssl)

target_link_libraries(lcy_ssl INTERFACE lcy)

find_package(OpenSSL REQUIRED)
target_link_libraries(lcy_ssl INTERFACE
    OpenSSL::SSL
    OpenSSL::Crypto
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/lcy
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.ipp"
)

install(
    TARGETS lcy lcy_ssl
    EXPORT lcy-targets
)

install(
    EXPORT lcy-targets
    FILE lcy-targets.cmake
    NAMESPACE lcy::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lcy
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lcy-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/lcy-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lcy
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/lcy-config-version.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/lcy-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/lcy-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lcy
)

option(LCY_BUILD_TESTS "Build lcy tests" ON)

if(LCY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()