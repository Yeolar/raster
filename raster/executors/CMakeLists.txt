# Copyright 2017 Yeolar

file(GLOB RASTER_EXECUTORS_SRCS *.cpp)
file(GLOB RASTER_EXECUTORS_HDRS *.h)

add_library(raster_executors OBJECT ${RASTER_EXECUTORS_SRCS})

install(FILES ${RASTER_EXECUTORS_HDRS} DESTINATION include/raster/executors)
