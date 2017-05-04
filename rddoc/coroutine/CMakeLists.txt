# Copyright (C) 2017, Yeolar

set(RDD_COROUTINE_SRCS
    Task.cpp
    TaskThread.cpp
    TaskThreadPool.cpp
)
add_library(rdd_coroutine OBJECT ${RDD_COROUTINE_SRCS})
