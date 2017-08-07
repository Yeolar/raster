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

install(FILES
    cxxfunctional.h
    TApplicationException.h
    TDispatchProcessor.h
    Thrift.h
    TProcessor.h
    protocol/TBase64Utils.h
    protocol/TBinaryProtocol.h
    protocol/TJSONProtocol.h
    protocol/TProtocolException.h
    protocol/TProtocol.h
    protocol/TVirtualProtocol.h
    transport/TBufferTransports.h
    transport/TServerSocket.h
    transport/TServerTransport.h
    transport/TSocket.h
    transport/TTransportException.h
    transport/TTransport.h
    transport/TVirtualTransport.h
    DESTINATION include/raster/3rd/thrift)
