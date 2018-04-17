# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_BINARY_SRCS
    AsyncClient.cpp
    Processor.cpp
    SyncClient.cpp
    SyncTransport.cpp
    Transport.cpp
)
add_library(raster_protocol_binary OBJECT ${RASTER_PROTOCOL_BINARY_SRCS})
add_dependencies(raster_protocol_binary accelerator)

install(FILES
    AsyncClient.h
    AsyncServer.h
    Processor.h
    SyncClient.h
    SyncTransport.h
    Transport.h
    DESTINATION include/raster/protocol/binary)
