# Copyright (C) 2017, Yeolar

add_executable(proxy
    gen-cpp/Proxy_constants.cpp
    gen-cpp/Proxy.cpp
    gen-cpp/Proxy_types.cpp
    Server.cpp
)
target_link_libraries(proxy raster_static)

add_executable(proxy-bench
    gen-cpp/Proxy_constants.cpp
    gen-cpp/Proxy.cpp
    gen-cpp/Proxy_types.cpp
    Bench.cpp
)
target_link_libraries(proxy-bench raster_static)

