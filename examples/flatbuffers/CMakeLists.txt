# Copyright (C) 2017, Yeolar

add_executable(flatbuffers
    Server.cpp
)
target_link_libraries(flatbuffers rdd_static)

add_executable(flatbuffers-bench
    Bench.cpp
)
target_link_libraries(flatbuffers-bench rdd_static)

