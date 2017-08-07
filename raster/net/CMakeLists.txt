# Copyright (C) 2017, Yeolar

set(RASTER_NET_SRCS
    Actor.cpp
    AsyncClient.cpp
    NetUtil.cpp
    Protocol.cpp
    Socket.cpp
)
add_library(raster_net OBJECT ${RASTER_NET_SRCS})
