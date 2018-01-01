# Copyright (C) 2017, Yeolar

set(RASTER_IO_EVENT_SRCS
    EPoll.cpp
    Event.cpp
    EventHandler.cpp
    EventLoop.cpp
    EventLoopManager.cpp
    EventPool.cpp
)
add_library(raster_io_event OBJECT ${RASTER_IO_EVENT_SRCS})

install(FILES
    EPoll.h
    Event.h
    EventHandler.h
    EventLoop.h
    EventLoopManager.h
    EventPool.h
    EventTask.h
    DESTINATION include/raster/io/event)
