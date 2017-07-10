# Copyright (C) 2017, Yeolar

add_executable(empty
    gen-cpp/Empty_constants.cpp
    gen-cpp/Empty.cpp
    gen-cpp/Empty_types.cpp
    Server.cpp
)
target_link_libraries(empty rdd_static)

add_executable(empty-bench
    gen-cpp/Empty_constants.cpp
    gen-cpp/Empty.cpp
    gen-cpp/Empty_types.cpp
    Bench.cpp
)
target_link_libraries(empty-bench rdd_static)

