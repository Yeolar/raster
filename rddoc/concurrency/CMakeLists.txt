# Copyright (C) 2017, Yeolar

set(RDD_CONCURRENCY_SRCS
    CPUThreadPool.cpp
    #    IOThreadPool.cpp
    ThreadPool.cpp
)
add_library(rdd_concurrency OBJECT ${RDD_CONCURRENCY_SRCS})
