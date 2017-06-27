# Copyright (C) 2017, Yeolar

set(RDD_NET_SRCS
    Actor.cpp
    AsyncClient.cpp
    NetUtil.cpp
    Protocol.cpp
    Socket.cpp
)
add_library(rdd_net OBJECT ${RDD_NET_SRCS})
