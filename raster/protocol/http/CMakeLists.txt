# Copyright (C) 2017, Yeolar

set(RASTER_PROTOCOL_HTTP_SRCS
    HTTP1xCodec.cpp
    HTTPCommonHeaders.cpp
    HTTPException.cpp
    HTTPHeaders.cpp
    HTTPMessage.cpp
    HTTPMethod.cpp
    ParseURL.cpp
    Processor.cpp
    RequestHandler.cpp
    Transport.cpp
    Util.cpp
)
add_library(raster_protocol_http OBJECT ${RASTER_PROTOCOL_HTTP_SRCS})

install(FILES
    AsyncClient.h
    AsyncServer.h
    HTTP1xCodec.h
    HTTPCommonHeaders.h
    HTTPEvent.h
    HTTPException.h
    HTTPHeaders.h
    HTTPMessage.h
    HTTPMethod.h
    ParseURL.h
    Processor.h
    Protocol.h
    RequestHandler.h
    ResponseBuilder.h
    SyncClient.h
    SyncTransport.h
    Transport.h
    Util.h
    DESTINATION include/raster/protocol/http)
