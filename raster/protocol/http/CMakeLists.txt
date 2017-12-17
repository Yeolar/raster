# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_HTTP_SRCS
    Cookie.cpp
    HTTPEvent.cpp
    HTTPHeaders.cpp
    HTTPRequest.cpp
    ParseQuery.cpp
    ParseURL.cpp
    Util.cpp
)
add_library(raster_protocol_http OBJECT ${RASTER_PROTOCOL_HTTP_SRCS})

install(FILES
    Cookie.h
    HTTPEvent.h
    HTTPHeaders.h
    HTTPRequest.h
    ParseQuery.h
    ParseURL.h
    Util.h
    DESTINATION include/raster/protocol/http)
