# Copyright (C) 2017, Yeolar

cmake_minimum_required(VERSION 3.2)

# Define project
project(raster VERSION 2.0.0)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake-scripts)

# Required packages
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(Boost_USE_MULTITHREADED TRUE)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS context random regex thread)
find_package(DoubleConversion REQUIRED)
find_package(GFlags REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Accelerator REQUIRED)

# Optional packages
find_package(GTest)

# Setup environment
option(BUILD_SHARED_LIBS "Build libraries as shared ones" OFF)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")    # memcheck
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Link libraries
link_libraries(
  ${ACCELERATOR_LIBRARY}
  ${Boost_LIBRARIES}
  ${DOUBLE_CONVERSION_LIBRARY}
  ${GFLAGS_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_PREFIX_PATH}/include
)

# Check and setup configurations
include(CheckCXXSourceCompiles REQUIRED)
include(CheckFunctionExists REQUIRED)

if(GFLAGS_FOUND)
  set(CMAKE_REQUIRED_LIBRARIES "gflags")

  check_cxx_source_compiles("
    #include <gflags/gflags.h>
    void foo() { google::GetArgv(); }
    int main() { return 0; }"
    RASTER_UNUSUAL_GFLAGS_NAMESPACE)

  if(RASTER_UNUSUAL_GFLAGS_NAMESPACE)
    set(RASTER_GFLAGS_NAMESPACE "google")
  else()
    set(RASTER_GFLAGS_NAMESPACE "gflags")
  endif()
endif()

configure_file(
  ${PROJECT_SOURCE_DIR}/raster/raster-config.h.in
  ${PROJECT_SOURCE_DIR}/raster/raster-config.h
)

# Work on all sub directories
add_subdirectory(raster/3rd/http_parser)
add_subdirectory(raster)
add_subdirectory(raster/coroutine)
add_subdirectory(raster/framework)
add_subdirectory(raster/net)
add_subdirectory(raster/protocol/binary)
add_subdirectory(raster/protocol/http)
add_subdirectory(raster/protocol/proto)

add_library(raster
  $<TARGET_OBJECTS:httpparser>
  $<TARGET_OBJECTS:raster_coroutine>
  $<TARGET_OBJECTS:raster_framework>
  $<TARGET_OBJECTS:raster_net>
  $<TARGET_OBJECTS:raster_protocol_binary>
  $<TARGET_OBJECTS:raster_protocol_http>
  $<TARGET_OBJECTS:raster_protocol_proto>
)

install(TARGETS raster
  EXPORT raster-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Binary
add_subdirectory(examples/http)
add_subdirectory(examples/pbrpc)

# Test
if(GTEST_FOUND)
    enable_testing()
#    add_subdirectory(raster/framework/test)
#    add_subdirectory(raster/protocol/http/test)
endif()

# CmakeConfig
include(CMakePackageConfigHelpers)

install(
  EXPORT raster-targets
  NAMESPACE raster::
  FILE raster-targets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raster
)

configure_package_config_file(
  ${CMAKE_MODULE_PATH}/raster-config.cmake.in
  raster-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raster
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/raster-config.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/raster
)

# Pack
set(CPACK_SYSTEM_NAME "amd64")
find_program(DPKG_CMD dpkg)
if(DPKG_CMD)
  execute_process(
    COMMAND "${DPKG_CMD}" --print-architecture
    OUTPUT_VARIABLE CPACK_SYSTEM_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

include(InstallRequiredSystemLibraries)
if(DPKG_CMD)
  set(CPACK_GENERATOR "DEB;TXZ")
else()
  set(CPACK_GENERATOR "TXZ")
endif()
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT "yeolar@qq.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A network and server framework.")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev,libgflags-dev,libssl-dev,zlib1g-dev")
include(CPack)

