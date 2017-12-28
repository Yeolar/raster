# Copyright (C) 2017, Yeolar

set(RASTER_COROUTINE_SRCS
    Fiber.cpp
    FiberManager.cpp
)
add_library(raster_coroutine OBJECT ${RASTER_COROUTINE_SRCS})

install(FILES
    BoostContext.h
    Context.h
    Fiber.h
    FiberManager.h
    FunctionTask.h
    DESTINATION include/raster/coroutine)
