# Copyright (C) 2017, Yeolar

set(RDD_PROTOCOL_PROTO_SRCS
    AsyncServer.cpp
    Message.cpp
    RpcController.cpp
)
add_library(rdd_protocol_proto OBJECT ${RDD_PROTOCOL_PROTO_SRCS})

