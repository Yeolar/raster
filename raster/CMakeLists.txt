# Copyright 2018 Yeolar

#file(GLOB RASTER_BASE_SRCS *.cpp)
file(GLOB RASTER_BASE_HDRS *.h)

#add_library(raster_base OBJECT ${RASTER_BASE_SRCS})

install(FILES ${RASTER_BASE_HDRS} DESTINATION include/raster)
