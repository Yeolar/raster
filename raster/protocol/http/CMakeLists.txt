# Copyright 2017 Yeolar

set(RASTER_PROTOCOL_HTTP_SRCS
    AsyncClient.cpp
    AsyncServer.cpp
    HTTP1xCodec.cpp
    HTTPCommonHeaders.cpp
    HTTPException.cpp
    HTTPHeaders.cpp
    HTTPMessage.cpp
    HTTPMethod.cpp
    ParseURL.cpp
    Processor.cpp
    RequestHandler.cpp
    SyncClient.cpp
    SyncTransport.cpp
    Transport.cpp
    Util.cpp
)
add_library(raster_protocol_http OBJECT ${RASTER_PROTOCOL_HTTP_SRCS})
add_dependencies(raster_protocol_http accelerator)

install(FILES
    AsyncClient.h
    AsyncServer.h
    HTTP1xCodec.h
    HTTPCommonHeaders.h
    HTTPException.h
    HTTPHeaders.h
    HTTPMessage.h
    HTTPMethod.h
    ParseURL.h
    Processor.h
    RequestHandler.h
    ResponseBuilder.h
    SyncClient.h
    SyncTransport.h
    Transport.h
    Util.h
    DESTINATION include/raster/protocol/http)
