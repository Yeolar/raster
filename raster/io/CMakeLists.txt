# Copyright (C) 2017, Yeolar

set(RASTER_IO_SRCS
    Cursor.cpp
    Descriptor.cpp
    File.cpp
    FileUtil.cpp
    FSUtil.cpp
    IOBuf.cpp
    IOBufQueue.cpp
    MMapIOAlloc.cpp
    Path.cpp
    RecordIO.cpp
    Waker.cpp
)
add_library(raster_io OBJECT ${RASTER_IO_SRCS})

install(FILES
    Cursor.h
    Cursor-inl.h
    Descriptor.h
    File.h
    FileUtil.h
    FlatDict.h
    FSUtil.h
    IOBuf.h
    IOBufQueue.h
    MMapFlatDict.h
    MMapIOAlloc.h
    Path.h
    RecordIO.h
    TypedIOBuf.h
    Waker.h
    DESTINATION include/raster/io)
