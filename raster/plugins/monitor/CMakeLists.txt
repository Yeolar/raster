# Copyright (C) 2017, Yeolar

set(RASTER_PLUGINS_MONITOR_SRCS
    FalconClient.cpp
    Monitor.cpp
)
add_library(raster_plugins_monitor OBJECT ${RASTER_PLUGINS_MONITOR_SRCS})
