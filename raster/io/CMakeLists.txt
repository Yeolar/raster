# Copyright (C) 2017, Yeolar

set(RASTER_IO_SRCS
    Compression.cpp
    Cursor.cpp
    Descriptor.cpp
    File.cpp
    FSUtil.cpp
    HugePages.cpp
    IOBuf.cpp
    IOBufQueue.cpp
    MMapIOAlloc.cpp
    Path.cpp
    RecordIO.cpp
    ZlibStreamCompressor.cpp
    ZlibStreamDecompressor.cpp
)
add_library(raster_io OBJECT ${RASTER_IO_SRCS})

install(FILES
    Compression.h
    Cursor.h
    Cursor-inl.h
    Descriptor.h
    File.h
    FileUtil.h
    FlatDict.h
    FSUtil.h
    HugePages.h
    IOBuf.h
    IOBufQueue.h
    MMapFlatDict.h
    MMapIOAlloc.h
    Path.h
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
