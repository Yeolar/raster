# Copyright (C) 2017, Yeolar

set(RDD_CONCURRENCY_SRCS
    TaskThread.cpp
    TaskThreadPool.cpp
)
add_library(rdd_concurrency OBJECT ${RDD_CONCURRENCY_SRCS})
