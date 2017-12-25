# Copyright (C) 2017, Yeolar

set(RASTER_NET_SRCS
    Actor.cpp
    AsyncClient.cpp
    ErrorEnum.cpp
    NetUtil.cpp
    Peer.cpp
    Socket.cpp
    Transport.cpp
)
add_library(raster_net OBJECT ${RASTER_NET_SRCS})

install(FILES
    Actor.h
    AsyncClientExecutor.h
    AsyncClient.h
    Channel.h
    ErrorEnum.h
    Exception.h
    NetUtil.h
    Peer.h
    Processor.h
    Service.h
    Socket.h
    Transport.h
    DESTINATION include/raster/net)
