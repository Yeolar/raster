# Copyright 2017 Yeolar

set(RASTER_PROTOCOL_PROTO_SRCS
    AsyncServer.cpp
    IOBufInputStream.cpp
    Message.cpp
    Processor.cpp
    RpcChannel.cpp
    RpcController.cpp
)
add_library(raster_protocol_proto OBJECT ${RASTER_PROTOCOL_PROTO_SRCS})
add_dependencies(raster_protocol_proto accelerator)

install(FILES
    AsyncClient.h
    AsyncClient-inl.h
    AsyncServer.h
    IOBufInputStream.h
    Message.h
    Processor.h
    RpcChannel.h
    RpcController.h
    SyncClient.h
    SyncClient-inl.h
    DESTINATION include/raster/protocol/proto)

add_executable(pbrpc_plugin
    plugin/RpcGenerator.cpp
    plugin/RpcPlugin.cpp
)
target_link_libraries(pbrpc_plugin
    ${Protobuf_PROTOC_LIBRARIES}
)
