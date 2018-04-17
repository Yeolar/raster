# Copyright (C) 2017, Yeolar

set(RASTER_FRAMEWORK_SRCS
    Config.cpp
    Degrader.cpp
    FalconSender.cpp
    HubAdaptor.cpp
    Sampler.cpp
    Signal.cpp
)
add_library(raster_framework OBJECT ${RASTER_FRAMEWORK_SRCS})
add_dependencies(raster_framework accelerator)

install(FILES
    Config.h
    Degrader.h
    FalconSender.h
    HubAdaptor.h
    Sampler.h
    Signal.h
    DESTINATION include/raster/framework)

add_executable(configutil
    ConfigUtil.cpp
)
target_link_libraries(configutil
    raster_static)
