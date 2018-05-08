# Copyright 2017 Yeolar

file(GLOB RASTER_PROTOCOL_HTTP_SRCS *.cpp)
file(GLOB RASTER_PROTOCOL_HTTP_HDRS *.h)
file(GLOB RASTER_PROTOCOL_HTTP_HDRS_EX HTTPCommonHeaders.template.h)
list(REMOVE_ITEM RASTER_PROTOCOL_HTTP_HDRS ${RASTER_PROTOCOL_HTTP_HDRS_EX})

add_library(raster_protocol_http OBJECT ${RASTER_PROTOCOL_HTTP_SRCS})
add_dependencies(raster_protocol_http accelerator)

install(FILES ${RASTER_PROTOCOL_HTTP_HDRS} DESTINATION include/raster/protocol/http)
