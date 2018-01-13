# Copyright (C) 2017, Yeolar

set(RASTER_CONCURRENCY_SRCS
    CPUThreadPoolExecutor.cpp
    IOThreadPoolExecutor.cpp
    ThreadedExecutor.cpp
    ThreadPoolExecutor.cpp
)
add_library(raster_concurrency OBJECT ${RASTER_CONCURRENCY_SRCS})

install(FILES
    CPUThreadPoolExecutor.h
    Executor.h
    IOThreadPoolExecutor.h
    ThreadedExecutor.h
    ThreadFactory.h
    ThreadPoolExecutor.h
    DESTINATION include/raster/concurrency)
