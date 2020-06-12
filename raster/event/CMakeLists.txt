# Copyright 2017 Yeolar

file(GLOB RASTER_EVENT_SRCS *.cpp)
file(GLOB RASTER_EVENT_HDRS *.h)

add_library(raster_event OBJECT ${RASTER_EVENT_SRCS})

install(FILES ${RASTER_EVENT_HDRS} DESTINATION include/raster/event)
