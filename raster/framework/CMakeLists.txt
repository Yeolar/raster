# Copyright 2017 Yeolar

file(GLOB RASTER_FRAMEWORK_SRCS *.cpp)
file(GLOB RASTER_FRAMEWORK_HDRS *.h)

add_library(raster_framework OBJECT ${RASTER_FRAMEWORK_SRCS})

install(FILES ${RASTER_FRAMEWORK_HDRS} DESTINATION include/raster/framework)

add_executable(configutil
    ConfigUtil.cpp
)
target_link_libraries(configutil
    raster_static)
