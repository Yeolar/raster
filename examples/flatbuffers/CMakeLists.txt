# Copyright (C) 2017, Yeolar

add_executable(flatbuffers
    Server.cpp
)
target_link_libraries(flatbuffers raster_static)

add_executable(flatbuffers-bench
    Bench.cpp
)
target_link_libraries(flatbuffers-bench raster_static)

