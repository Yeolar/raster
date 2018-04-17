# Copyright (C) 2017, Yeolar

set(RASTER_CONCURRENCY_SRCS
    IOThreadPoolExecutor.cpp
)
add_library(raster_concurrency OBJECT ${RASTER_CONCURRENCY_SRCS})

install(FILES
    IOThreadPoolExecutor.h
    DESTINATION include/raster/concurrency)
