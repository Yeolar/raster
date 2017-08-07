# Copyright (C) 2017, Yeolar

set(RASTER_IO_SRCS
    Compression.cpp
    HugePages.cpp
    IOBuf.cpp
    ZlibStreamCompressor.cpp
    ZlibStreamDecompressor.cpp
)
add_library(raster_io OBJECT ${RASTER_IO_SRCS})

add_executable(hugepageutil
    HugePageUtil.cpp
)
target_link_libraries(hugepageutil raster_static)

