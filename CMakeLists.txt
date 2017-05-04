# Copyright (C) 2017, Yeolar

# Setup compiler
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

# Define project
project(rddoc)

cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake-scripts")

# Set project version
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

# Required packages
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem regex system)
find_package(CURL REQUIRED)
find_package(GFlags REQUIRED)
find_package(ICU REQUIRED)
find_package(LevelDB REQUIRED)
find_package(Libmemcached REQUIRED)
find_package(MySQL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Optional packages
find_package(GTest)
find_package(GMock)

# Setup environment
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -std=c++11 -fPIC")

# Link libraries
link_libraries(
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${ICU_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
    ${LevelDB_LIBRARIES}
    ${LIBEVENT_LIB}
    ${LIBMEMCACHED_LIBRARY}
    ${MYSQL_CLIENT_LIBS}
    ${OPENSSL_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

include_directories(
    ${PROJECT_SOURCE_DIR}/rddoc/3rd
    ${PROJECT_SOURCE_DIR}
)

# Check and setup configurations
include(CheckCXXSourceCompiles REQUIRED)

check_cxx_source_compiles("
    #include <string.h>
    #include <errno.h>
    int main() {
        char buf[1024];
        buf[0] = 0;
        int ret = strerror_r(ENOMEM, buf, sizeof(buf));
        return ret;
    }"
    RDD_HAVE_XSI_STRERROR_R)

# Work on all sub directories
add_subdirectory(rddoc/coroutine)
add_subdirectory(rddoc/enc)
add_subdirectory(rddoc/framework)
add_subdirectory(rddoc/io)
add_subdirectory(rddoc/net)
add_subdirectory(rddoc/plugins/leveldb)
add_subdirectory(rddoc/plugins/memcached)
add_subdirectory(rddoc/plugins/monitor)
add_subdirectory(rddoc/plugins/mysql)
add_subdirectory(rddoc/util)

add_subdirectory(rddoc/3rd/double-conversion)
add_subdirectory(rddoc/3rd/http_parser)
add_subdirectory(rddoc/3rd/srilm)
add_subdirectory(rddoc/3rd/thrift)

add_library(rdd_static STATIC
    $<TARGET_OBJECTS:rdd_coroutine>
    $<TARGET_OBJECTS:rdd_enc>
    $<TARGET_OBJECTS:rdd_framework>
    $<TARGET_OBJECTS:rdd_io>
    $<TARGET_OBJECTS:rdd_net>
    $<TARGET_OBJECTS:rdd_plugins_leveldb>
    $<TARGET_OBJECTS:rdd_plugins_memcached>
    $<TARGET_OBJECTS:rdd_plugins_monitor>
    $<TARGET_OBJECTS:rdd_plugins_mysql>
    $<TARGET_OBJECTS:rdd_util>
    $<TARGET_OBJECTS:double-conversion>
    $<TARGET_OBJECTS:httpparser>
    $<TARGET_OBJECTS:srilm>
    $<TARGET_OBJECTS:thrift>
)
set_target_properties(rdd_static PROPERTIES OUTPUT_NAME rdd)
add_library(rdd_shared SHARED
    $<TARGET_OBJECTS:rdd_coroutine>
    $<TARGET_OBJECTS:rdd_enc>
    $<TARGET_OBJECTS:rdd_framework>
    $<TARGET_OBJECTS:rdd_io>
    $<TARGET_OBJECTS:rdd_net>
    $<TARGET_OBJECTS:rdd_plugins_leveldb>
    $<TARGET_OBJECTS:rdd_plugins_memcached>
    $<TARGET_OBJECTS:rdd_plugins_monitor>
    $<TARGET_OBJECTS:rdd_plugins_mysql>
    $<TARGET_OBJECTS:rdd_util>
    $<TARGET_OBJECTS:double-conversion>
    $<TARGET_OBJECTS:httpparser>
    $<TARGET_OBJECTS:srilm>
    $<TARGET_OBJECTS:thrift>
)
set_target_properties(rdd_shared PROPERTIES OUTPUT_NAME rdd)

# Binary
add_subdirectory(empty-thrift)

if(GTEST_FOUND AND GMOCK_FOUND)
    enable_testing()
    add_subdirectory(rddoc/enc/test)
    add_subdirectory(rddoc/gen/test)
    add_subdirectory(rddoc/io/test)
    add_subdirectory(rddoc/plugins/mysql/test)
    add_subdirectory(rddoc/util/test)
endif()

