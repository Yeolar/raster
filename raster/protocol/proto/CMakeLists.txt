# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_PROTO_SRCS
    AsyncServer.cpp
    Message.cpp
    RpcChannel.cpp
    RpcController.cpp
)
add_library(raster_protocol_proto OBJECT ${RASTER_PROTOCOL_PROTO_SRCS})

