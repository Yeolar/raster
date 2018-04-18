# Copyright 2017 Yeolar

set(RASTER_COROUTINE_SRCS
    Fiber.cpp
    FiberHub.cpp
    FiberManager.cpp
)
add_library(raster_coroutine OBJECT ${RASTER_COROUTINE_SRCS})
add_dependencies(raster_coroutine accelerator)

install(FILES
    BoostContext.h
    Context.h
    Fiber.h
    FiberHub.h
    FiberManager.h
    DESTINATION include/raster/coroutine)
