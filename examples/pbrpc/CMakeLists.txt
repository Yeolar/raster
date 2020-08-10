# Copyright (C) 2017, Yeolar

set(EXAMPLES_PBRPC_PROTOS
    Proxy.proto
)

foreach(proto ${EXAMPLES_PBRPC_PROTOS})
    execute_process(COMMAND
        protoc --cpp_out=${PROJECT_SOURCE_DIR}
               --proto_path=${PROJECT_SOURCE_DIR}
               ${PROJECT_SOURCE_DIR}/examples/pbrpc/${proto})
#    execute_process(COMMAND
#        protoc --plugin=protoc-gen-pbrpc_plugin=${PROJECT_BINARY_DIR}/raster/protocol/proto/pbrpc_plugin
#               --pbrpc_plugin_out=${PROJECT_SOURCE_DIR}
#               --proto_path=${PROJECT_SOURCE_DIR}
#               ${PROJECT_SOURCE_DIR}/examples/pbrpc/${proto})
endforeach()

add_executable(pbrpc
    Proxy.pb.cc
    Server.cpp
)
target_link_libraries(pbrpc raster)

add_executable(pbrpc-bench
    Proxy.pb.cc
    Bench.cpp
)
target_link_libraries(pbrpc-bench raster)

