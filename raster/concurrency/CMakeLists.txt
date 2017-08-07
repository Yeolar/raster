# Copyright (C) 2017, Yeolar

set(RASTER_CONCURRENCY_SRCS
    CPUThreadPool.cpp
    IOThreadPool.cpp
    ThreadPool.cpp
)
add_library(raster_concurrency OBJECT ${RASTER_CONCURRENCY_SRCS})

install(FILES
    CPUThreadPool.h
    IOThreadPool.h
    Observable.h
    Observer.h
    ThreadFactory.h
    ThreadPool.h
    DESTINATION include/raster/concurrency)
