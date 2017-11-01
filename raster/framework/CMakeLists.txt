# Copyright (C) 2017, Yeolar

set(RASTER_FRAMEWORK_SRCS
    Config.cpp
    FalconSender.cpp
    Monitor.cpp
)
add_library(raster_framework OBJECT ${RASTER_FRAMEWORK_SRCS})

install(FILES
    AutoTask.h
    Config.h
    FalconSender.h
    Monitor.h
    DESTINATION include/raster/framework)

add_executable(configutil
    ConfigUtil.cpp
)
target_link_libraries(configutil raster_static)
