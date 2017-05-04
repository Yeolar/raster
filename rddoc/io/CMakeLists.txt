# Copyright (C) 2017, Yeolar

set(RDD_IO_SRCS
    Compression.cpp
    IOBuf.cpp
    ZlibStreamCompressor.cpp
    ZlibStreamDecompressor.cpp
)
add_library(rdd_io OBJECT ${RDD_IO_SRCS})

