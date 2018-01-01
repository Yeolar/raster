# Copyright (C) 2017, Yeolar

set(RASTER_NET_SRCS
    Acceptor.cpp
    AsyncClient.cpp
    ErrorEnum.cpp
    Group.cpp
    NetHub.cpp
    NetUtil.cpp
    Peer.cpp
    Socket.cpp
    Transport.cpp
)
add_library(raster_net OBJECT ${RASTER_NET_SRCS})

install(FILES
    Acceptor.h
    AsyncClient.h
    Channel.h
    ErrorEnum.h
    Exception.h
    Group.h
    NetHub.h
    NetUtil.h
    Peer.h
    Processor.h
    Service.h
    Socket.h
    Transport.h
    DESTINATION include/raster/net)
