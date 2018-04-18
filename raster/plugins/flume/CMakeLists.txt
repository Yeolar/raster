# Copyright 2017 Yeolar

set(RASTER_PLUGINS_FLUME_SRCS
    gen-cpp/scribe_constants.cpp
    gen-cpp/Scribe.cpp
    gen-cpp/scribe_types.cpp
    FlumeClient.cpp
)
add_library(raster_plugins_flume OBJECT ${RASTER_PLUGINS_FLUME_SRCS})
add_dependencies(raster_plugins_flume accelerator)

install(FILES
    FlumeClient.h
    DESTINATION include/raster/plugins/flume)
