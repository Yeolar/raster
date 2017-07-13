# Copyright (C) 2017, Yeolar

add_executable(pbrpc
    Empty.pb.cc
    Server.cpp
)
target_link_libraries(pbrpc rdd_static)

#add_executable(pbrpc-bench
#    gen-cpp/pbrpc_constants.cpp
#    gen-cpp/pbrpc.cpp
#    gen-cpp/pbrpc_types.cpp
#    Bench.cpp
#)
#target_link_libraries(pbrpc-bench rdd_static)
#
