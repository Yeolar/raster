# Copyright (C) 2017, Yeolar

# Setup compiler
#set(CMAKE_C_COMPILER gcc)
#set(CMAKE_CXX_COMPILER g++)

# Define project
project(raster)

cmake_minimum_required(VERSION 3.2)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake-scripts")

# Set project version
set(VERSION_MAJOR 1)
set(VERSION_MINOR 2)
set(VERSION_PATCH 1)

# Required packages
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS context regex)
find_package(CURL REQUIRED)
find_package(GFlags REQUIRED)
find_package(ICU REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)

# Optional packages
find_package(GTest)

# Setup environment
set(CMAKE_BUILD_TYPE Release)   # Debug: -g; Release: -O3 -DNDEBUG
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -mpclmul")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")    # memcheck
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Add external
include(accelerator)
include(flatbuffers)

# Link libraries
link_libraries(
    ${accelerator_STATIC_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${GFLAGS_LIBRARIES}
    ${ICU_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

include_directories(
    ${accelerator_INCLUDE_DIR}
    ${flatbuffers_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/raster/3rd
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

if(GFLAGS_FOUND)
    set(CMAKE_REQUIRED_LIBRARIES "gflags")

    check_cxx_source_compiles("
        #include <gflags/gflags.h>
        void foo() { google::GetArgv(); }
        int main() { return 0; }"
        RDD_UNUSUAL_GFLAGS_NAMESPACE)

    if(RDD_UNUSUAL_GFLAGS_NAMESPACE)
        set(RDD_GFLAGS_NAMESPACE "google")
    else()
        set(RDD_GFLAGS_NAMESPACE "gflags")
    endif()
endif()

configure_file(
    "${PROJECT_SOURCE_DIR}/raster/raster-config.h.in"
    "${PROJECT_SOURCE_DIR}/raster/raster-config.h"
)

# Work on all sub directories
add_subdirectory(raster/3rd/http_parser)
add_subdirectory(raster/3rd/thrift)

add_subdirectory(raster/coroutine)
add_subdirectory(raster/framework)
add_subdirectory(raster/net)
add_subdirectory(raster/plugins/flume)
add_subdirectory(raster/protocol/binary)
add_subdirectory(raster/protocol/http)
add_subdirectory(raster/protocol/proto)
add_subdirectory(raster/protocol/thrift)
add_subdirectory(raster/serializer)

add_library(raster_static STATIC
    $<TARGET_OBJECTS:httpparser>
    $<TARGET_OBJECTS:thrift>
    $<TARGET_OBJECTS:raster_coroutine>
    $<TARGET_OBJECTS:raster_framework>
    $<TARGET_OBJECTS:raster_net>
    $<TARGET_OBJECTS:raster_plugins_flume>
    $<TARGET_OBJECTS:raster_protocol_binary>
    $<TARGET_OBJECTS:raster_protocol_http>
    $<TARGET_OBJECTS:raster_protocol_proto>
    $<TARGET_OBJECTS:raster_protocol_thrift>
)
set_target_properties(raster_static PROPERTIES OUTPUT_NAME raster)

add_library(raster_shared SHARED
    $<TARGET_OBJECTS:httpparser>
    $<TARGET_OBJECTS:thrift>
    $<TARGET_OBJECTS:raster_coroutine>
    $<TARGET_OBJECTS:raster_framework>
    $<TARGET_OBJECTS:raster_net>
    $<TARGET_OBJECTS:raster_plugins_flume>
    $<TARGET_OBJECTS:raster_protocol_binary>
    $<TARGET_OBJECTS:raster_protocol_http>
    $<TARGET_OBJECTS:raster_protocol_proto>
    $<TARGET_OBJECTS:raster_protocol_thrift>
)
set_target_properties(raster_shared PROPERTIES OUTPUT_NAME raster)

install(TARGETS raster_static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE
                GROUP_READ
                WORLD_READ
)
install(TARGETS raster_shared
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

# Binary
add_subdirectory(examples/empty)
#add_subdirectory(examples/flatbuffers)
add_subdirectory(examples/http)
#add_subdirectory(examples/parallel)
add_subdirectory(examples/pbrpc)
add_subdirectory(examples/proxy)

# Test
if(GTEST_FOUND)
    enable_testing()
#    add_subdirectory(raster/framework/test)
#    add_subdirectory(raster/gen/test)
#    add_subdirectory(raster/parallel/test)
#    add_subdirectory(raster/protocol/http/test)
#    add_subdirectory(raster/serializer/test)
endif()

# Pack
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_GENERATOR "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_PACKAGE_DESCRIPTION "raster: network and server framework of rddoc.")
SET(CPACK_PACKAGE_CONTACT "rdd@rddoc.com")
include(CPack)

