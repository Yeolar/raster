# Copyright 2017 Yeolar

file(GLOB RASTER_CONCURRENCY_SRCS *.cpp)
file(GLOB RASTER_CONCURRENCY_HDRS *.h)

add_library(raster_concurrency OBJECT ${RASTER_CONCURRENCY_SRCS})

install(FILES ${RASTER_CONCURRENCY_HDRS} DESTINATION include/raster/concurrency)
