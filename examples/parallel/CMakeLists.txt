# Copyright (C) 2017, Yeolar

add_executable(parallel
    gen-cpp/Parallel_constants.cpp
    gen-cpp/Parallel.cpp
    gen-cpp/Parallel_types.cpp
    Server.cpp
)
target_link_libraries(parallel
    accelerator_static
    raster_static)

add_executable(parallel-bench
    gen-cpp/Parallel_constants.cpp
    gen-cpp/Parallel.cpp
    gen-cpp/Parallel_types.cpp
    Bench.cpp
)
target_link_libraries(parallel-bench
    accelerator_static
    raster_static)

