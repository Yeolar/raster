# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_BINARY_SRCS
    Transport.cpp
)
add_library(raster_protocol_binary OBJECT ${RASTER_PROTOCOL_BINARY_SRCS})

install(FILES
    AsyncClient.h
    AsyncServer.h
    Processor.h
    SyncClient.h
    SyncTransport.h
    Transport.h
    DESTINATION include/raster/protocol/binary)
