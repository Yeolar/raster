# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_HTTP_SRCS
    Cookie.cpp
    HTTPEvent.cpp
    HTTPHeaders.cpp
    HTTPMethod.cpp
    HTTPRequest.cpp
    ParseQuery.cpp
    ParseURL.cpp
    Processor.cpp
    Util.cpp
)
add_library(raster_protocol_http OBJECT ${RASTER_PROTOCOL_HTTP_SRCS})

install(FILES
    Cookie.h
    HTTPEvent.h
    HTTPHeaders.h
    HTTPMethod.h
    HTTPRequest.h
    HTTPResponse.h
    ParseQuery.h
    ParseURL.h
    Processor.h
    Protocol.h
    Util.h
    DESTINATION include/raster/protocol/http)
