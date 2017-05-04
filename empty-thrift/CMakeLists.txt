# Copyright (C) 2017, Yeolar

add_executable(empty-thrift
    gen-cpp/Empty_constants.cpp
    gen-cpp/Empty.cpp
    gen-cpp/Empty_types.cpp
    Server.cpp
)
target_link_libraries(empty-thrift rdd_static)

add_executable(empty-thrift-bench
    gen-cpp/Empty_constants.cpp
    gen-cpp/Empty.cpp
    gen-cpp/Empty_types.cpp
    Bench.cpp
)
target_link_libraries(empty-thrift-bench rdd_static)

