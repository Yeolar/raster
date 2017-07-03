# Copyright (C) 2017, Yeolar

set(RDD_IO_EVENT_SRCS
    Event.cpp
    EventHandler.cpp
    EventLoop.cpp
    EventLoopManager.cpp
    EventPool.cpp
    Poll.cpp
)
add_library(rdd_io_event OBJECT ${RDD_IO_EVENT_SRCS})

