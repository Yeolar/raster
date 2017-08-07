# Copyright (C) 2017, Yeolar

add_executable(pbrpc
    Proxy.pb.cc
    Server.cpp
)
target_link_libraries(pbrpc raster_static)

add_executable(pbrpc-bench
    Proxy.pb.cc
    Bench.cpp
)
target_link_libraries(pbrpc-bench raster_static)

