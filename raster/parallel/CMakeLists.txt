# Copyright (C) 2017, Yeolar

set(RASTER_PARALLEL_SRCS
    DAG.cpp
    Scheduler.cpp
)
add_library(raster_parallel OBJECT ${RASTER_PARALLEL_SRCS})

install(FILES
    DAG.h
    Graph.h
    Scheduler.h
    DESTINATION include/raster/parallel)
