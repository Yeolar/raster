# Copyright 2017 Yeolar

file(GLOB RASTER_PROTOCOL_THRIFT_SRCS *.cpp)
file(GLOB RASTER_PROTOCOL_THRIFT_HDRS *.h)

add_library(raster_protocol_thrift OBJECT ${RASTER_PROTOCOL_THRIFT_SRCS})
add_dependencies(raster_protocol_thrift accelerator)

install(FILES ${RASTER_PROTOCOL_THRIFT_HDRS} DESTINATION include/raster/protocol/thrift)
