# Copyright (C) 2017, Yeolar

include_directories(
    ${PROJECT_SOURCE_DIR}/examples/flatbuffers
)

add_executable(flatbuffers
    Server.cpp
)
target_link_libraries(flatbuffers raster_static)

add_executable(flatbuffers-bench
    Bench.cpp
)
target_link_libraries(flatbuffers-bench raster_static)

