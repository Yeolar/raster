# Copyright (C) 2017, Yeolar

set(RASTER_COROUTINE_SRCS
    Fiber.cpp
    FiberManager.cpp
)
add_library(raster_coroutine OBJECT ${RASTER_COROUTINE_SRCS})
