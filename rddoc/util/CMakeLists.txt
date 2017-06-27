# Copyright (C) 2017, Yeolar

set(RDD_UTIL_SRCS
    Arena.cpp
    Checksum.cpp
    Conv.cpp
    Demangle.cpp
    dynamic.cpp
    Futex.cpp
    json.cpp
    #MemTrace.cpp
    Range.cpp
    Sha.cpp
    Signal.cpp
    SpookyHashV2.cpp
    String.cpp
    SysUtil.cpp
    TestUtil.cpp
    Thread.cpp
    Time.cpp
    Unicode.cpp
    Utf8StringPiece.cpp
    Uuid.cpp
)
add_library(rdd_util OBJECT ${RDD_UTIL_SRCS})

