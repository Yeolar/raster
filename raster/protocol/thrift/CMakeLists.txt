# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_THRIFT_SRCS
    Processor.cpp
    Util.cpp
)
add_library(raster_protocol_thrift OBJECT ${RASTER_PROTOCOL_THRIFT_SRCS})

install(FILES
    AsyncClient.h
    AsyncClient-inl.h
    AsyncServer.h
    Processor.h
    SyncClient.h
    SyncClient-inl.h
    Util.h
    DESTINATION include/raster/protocol/thrift)
