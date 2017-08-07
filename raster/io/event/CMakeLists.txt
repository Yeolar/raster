# Copyright (C) 2017, Yeolar

set(RASTER_IO_EVENT_SRCS
    Event.cpp
    EventHandler.cpp
    EventLoop.cpp
    EventLoopManager.cpp
    EventPool.cpp
    Poll.cpp
)
add_library(raster_io_event OBJECT ${RASTER_IO_EVENT_SRCS})

install(FILES
    EventExecutor.h
    Event.h
    EventHandler.h
    EventLoop.h
    EventLoopManager.h
    EventPool.h
    Poll.h
    DESTINATION include/raster/io/event)
