# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_HTTP_SRCS
    Cookie.cpp
    HTTPHeaders.cpp
    ParseQuery.cpp
    ParseURL.cpp
    Util.cpp
)
add_library(raster_protocol_http OBJECT ${RASTER_PROTOCOL_HTTP_SRCS})

install(FILES
    Cookie.h
    HTTPHeaders.h
    ParseQuery.h
    ParseURL.h
    Util.h
    DESTINATION include/raster/protocol/http)
