# Copyright (C) 2017, Yeolar

set(RASTER_NET_SRCS
    Actor.cpp
    AsyncClient.cpp
    ErrorEnum.cpp
    NetUtil.cpp
    Protocol.cpp
    Socket.cpp
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
    Processor.h
    Protocol.h
    Service.h
    Socket.h
    DESTINATION include/raster/net)
