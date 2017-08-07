# Copyright (C) 2017, Yeolar

set(RASTER_PLUGINS_MEMCACHED_SRCS
    MemcachedClient.cpp
)
add_library(raster_plugins_memcached OBJECT ${RASTER_PLUGINS_MEMCACHED_SRCS})
