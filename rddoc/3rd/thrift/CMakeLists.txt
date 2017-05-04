# Copyright (C) 2017, Yeolar

set(THRIFT_SRCS
    TApplicationException.cpp
    protocol/TBase64Utils.cpp
    protocol/TJSONProtocol.cpp
    protocol/TProtocol.cpp
    transport/TBufferTransports.cpp
    transport/TServerSocket.cpp
    transport/TSocket.cpp
    transport/TTransportException.cpp
)
add_library(thrift OBJECT ${THRIFT_SRCS})

