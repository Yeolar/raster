# Copyright (C) 2017, Yeolar

set(RASTER_FRAMEWORK_SRCS
    Config.cpp
    FalconSender.cpp
    Monitor.cpp
    Signal.cpp
)
add_library(raster_framework OBJECT ${RASTER_FRAMEWORK_SRCS})

install(FILES
    AutoTask.h
    Config.h
    Degrader.h
    FalconSender.h
    Monitor.h
    Sampler.h
    Signal.h
    DESTINATION include/raster/framework)

add_executable(configutil
    ConfigUtil.cpp
)
target_link_libraries(configutil raster_static)
