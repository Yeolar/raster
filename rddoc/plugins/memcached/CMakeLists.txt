# Copyright (C) 2017, Yeolar

set(RDD_PLUGINS_MEMCACHED_SRCS
    MemcachedClient.cpp
)
add_library(rdd_plugins_memcached OBJECT ${RDD_PLUGINS_MEMCACHED_SRCS})
