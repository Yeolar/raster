# Copyright (C) 2017, Yeolar

set(RASTER_IO_SRCS
    Compression.cpp
    Cursor.cpp
    HugePages.cpp
    IOBuf.cpp
    MMapIOAlloc.cpp
    RecordIO.cpp
    ZlibStreamCompressor.cpp
    ZlibStreamDecompressor.cpp
)
add_library(raster_io OBJECT ${RASTER_IO_SRCS})

install(FILES
    Compression.h
    Cursor.h
    Descriptor.h
    File.h
    FileUtil.h
    FlatDict.h
    FSUtil.h
    HugePages.h
    IOBuf.h
    MMapFlatDict.h
    MMapIOAlloc.h
    RecordIO.h
    TypedIOBuf.h
    Waker.h
    ZlibStreamCompressor.h
    ZlibStreamDecompressor.h
    DESTINATION include/raster/io)

add_executable(hugepageutil
    HugePageUtil.cpp
)
target_link_libraries(hugepageutil raster_static)
