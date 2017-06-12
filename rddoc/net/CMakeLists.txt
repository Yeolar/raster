# Copyright (C) 2017, Yeolar

set(RDD_NET_SRCS
    Actor.cpp
    AsyncClient.cpp
    Event.cpp
    EventGroup.cpp
    EventHandler.cpp
    EventLoop.cpp
    EventPool.cpp
    EventTask.cpp
    NetUtil.cpp
    Poll.cpp
    Protocol.cpp
    Socket.cpp
)
add_library(rdd_net OBJECT ${RDD_NET_SRCS})
