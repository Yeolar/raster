# Copyright (C) 2017, Yeolar

set(RASTER_SSL_SRCS
    OpenSSL.cpp
)
add_library(raster_ssl OBJECT ${RASTER_SSL_SRCS})

install(FILES
    OpenSSL.h
    OpenSSLHash.h
    OpenSSLPtrTypes.h
    DESTINATION include/raster/ssl)
