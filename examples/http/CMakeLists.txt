# Copyright (C) 2017, Yeolar

add_executable(httpserver
    Server.cpp
)
target_link_libraries(httpserver raster)

add_executable(httpserver-bench
    Bench.cpp
)
target_link_libraries(httpserver-bench raster)

