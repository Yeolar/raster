# Copyright 2017 Yeolar

file(GLOB RASTER_COROUTINE_SRCS *.cpp)
file(GLOB RASTER_COROUTINE_HDRS *.h)

add_library(raster_coroutine OBJECT ${RASTER_COROUTINE_SRCS})
add_dependencies(raster_coroutine accelerator)

install(FILES ${RASTER_COROUTINE_HDRS} DESTINATION include/raster/coroutine)
