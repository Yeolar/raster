# Copyright 2017 Yeolar

file(GLOB RASTER_PROTOCOL_BINARY_SRCS *.cpp)
file(GLOB RASTER_PROTOCOL_BINARY_HDRS *.h)

add_library(raster_protocol_binary OBJECT ${RASTER_PROTOCOL_BINARY_SRCS})
add_dependencies(raster_protocol_binary accelerator)

install(FILES ${RASTER_PROTOCOL_BINARY_HDRS} DESTINATION include/raster/protocol/binary)
