# Copyright (C) 2017, Yeolar

add_executable(empty-http
    gen-cpp/Empty_constants.cpp
    gen-cpp/Empty.cpp
    gen-cpp/Empty_types.cpp
    Server.cpp
)
target_link_libraries(empty-http rdd_static)

add_executable(empty-http-bench
    gen-cpp/Empty_constants.cpp
    gen-cpp/Empty.cpp
    gen-cpp/Empty_types.cpp
    Bench.cpp
)
target_link_libraries(empty-http-bench rdd_static)

