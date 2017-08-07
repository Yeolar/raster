# Copyright (C) 2017, Yeolar

set(RASTER_FRAMEWORK_SRCS
    Config.cpp
    Monitor.cpp
)
add_library(raster_framework OBJECT ${RASTER_FRAMEWORK_SRCS})

install(FILES
    AutoTask.h
    Config.h
    Monitor.h
    DESTINATION include/raster/framework)
