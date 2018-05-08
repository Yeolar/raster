# Copyright 2017 Yeolar

file(GLOB_RECURSE THRIFT_SRCS *.cpp)
file(GLOB THRIFT_HDRS *.h)
file(GLOB THRIFT_PROTOCOL_HDRS protocol/*.h protocol/*.tcc)
file(GLOB THRIFT_TRANSPORT_HDRS transport/*.h)

add_library(thrift OBJECT ${THRIFT_SRCS})
add_dependencies(thrift accelerator)

install(FILES ${THRIFT_HDRS} DESTINATION include/raster/3rd/thrift)
install(FILES ${THRIFT_PROTOCOL_HDRS} DESTINATION include/raster/3rd/thrift/protocol)
install(FILES ${THRIFT_TRANSPORT_HDRS} DESTINATION include/raster/3rd/thrift/transport)
