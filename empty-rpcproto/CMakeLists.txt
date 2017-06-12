# Copyright (C) 2017, Yeolar

add_executable(empty-proto
    Empty.pb.cc
    Server.cpp
)
target_link_libraries(empty-proto rdd_static)

add_executable(empty-proto-bench
    Empty.pb.cc
    Bench.cpp
)
target_link_libraries(empty-proto-bench rdd_static)

