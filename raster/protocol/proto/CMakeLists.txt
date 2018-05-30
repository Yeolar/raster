# Copyright 2017 Yeolar

file(GLOB RASTER_PROTOCOL_PROTO_SRCS *.cpp)
file(GLOB RASTER_PROTOCOL_PROTO_HDRS *.h)

add_library(raster_protocol_proto OBJECT ${RASTER_PROTOCOL_PROTO_SRCS})

install(FILES ${RASTER_PROTOCOL_PROTO_HDRS} DESTINATION include/raster/protocol/proto)

add_executable(pbrpc_plugin
    plugin/RpcGenerator.cpp
    plugin/RpcPlugin.cpp
)
target_link_libraries(pbrpc_plugin
    ${PROTOBUF_PROTOC_LIBRARIES}
)
