# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_HTTP_SRCS
    Cookie.cpp
    HTTPCommonHeaders.cpp
    HTTPEvent.cpp
    HTTPException.cpp
    HTTPHeaders.cpp
    HTTPMethod.cpp
    HTTPRequest.cpp
    HTTPResponse.cpp
    ParseQuery.cpp
    ParseURL.cpp
    Processor.cpp
    RequestHandler.cpp
    Util.cpp
)
add_library(raster_protocol_http OBJECT ${RASTER_PROTOCOL_HTTP_SRCS})

install(FILES
    Cookie.h
    HTTPCommonHeaders.h
    HTTPEvent.h
    HTTPException.h
    HTTPHeaders.h
    HTTPMethod.h
    HTTPRequest.h
    HTTPResponse.h
    ParseQuery.h
    ParseURL.h
    Processor.h
    Protocol.h
    RequestHandler.h
    Util.h
    DESTINATION include/raster/protocol/http)
