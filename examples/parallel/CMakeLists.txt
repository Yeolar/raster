# Copyright (C) 2017, Yeolar

add_executable(parallel
    gen-cpp/Empty_constants.cpp
    gen-cpp/Empty.cpp
    gen-cpp/Empty_types.cpp
    Server.cpp
)
target_link_libraries(parallel rdd_static)

add_executable(parallel-bench
    gen-cpp/Empty_constants.cpp
    gen-cpp/Empty.cpp
    gen-cpp/Empty_types.cpp
    Bench.cpp
)
target_link_libraries(parallel-bench rdd_static)

