# Copyright 2017 Yeolar

set(RASTER_NET_SRCS
    Acceptor.cpp
    AsyncClient.cpp
    ErrorEnum.cpp
    Event.cpp
    EventHandler.cpp
    EventPool.cpp
    Group.cpp
    NetHub.cpp
    NetUtil.cpp
    Peer.cpp
    Socket.cpp
    Transport.cpp
)
add_library(raster_net OBJECT ${RASTER_NET_SRCS})
add_dependencies(raster_net accelerator)

install(FILES
    Acceptor.h
    AsyncClient.h
    Channel.h
    ErrorEnum.h
    Event.h
    EventHandler.h
    EventPool.h
    EventTask.h
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
