# Copyright (C) 2017, Yeolar

set(RASTER_IO_COMPRESSION_SRCS
    Compression.cpp
    ZlibStreamCompressor.cpp
    ZlibStreamDecompressor.cpp
)
add_library(raster_io_compression OBJECT ${RASTER_IO_COMPRESSION_SRCS})

install(FILES
    Compression.h
    ZlibStreamCompressor.h
    ZlibStreamDecompressor.h
    DESTINATION include/raster/io/compression)
