/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

namespace rdd {

// Max must be the last one.
#define RDD_NET_ERROR_GEN(x)                    \
    x(None),                                    \
    x(Message),                                 \
    x(Connect),                                 \
    x(ConnectTimeout),                          \
    x(Read),                                    \
    x(Write),                                   \
    x(Timeout),                                 \
    x(Handshake),                               \
    x(NoServer),                                \
    x(MaxRedirects),                            \
    x(InvalidRedirect),                         \
    x(ResponseAction),                          \
    x(MaxConnects),                             \
    x(Dropped),                                 \
    x(Connection),                              \
    x(ConnectionReset),                         \
    x(ParseHeader),                             \
    x(ParseBody),                               \
    x(EOF),                                     \
    x(ClientRenegotiation),                     \
    x(Unknown),                                 \
    x(BadDecompress),                           \
    x(SSL),                                     \
    x(StreamAbort),                             \
    x(StreamUnacknowledged),                    \
    x(WriteTimeout),                            \
    x(AddressPrivate),                          \
    x(AddressFamilyNotSupported),               \
    x(DNSNoResults),                            \
    x(MalformedInput),                          \
    x(UnsupportedExpectation),                  \
    x(MethodNotSupported),                      \
    x(UnsupportedScheme),                       \
    x(Shutdown),                                \
    x(IngressStateTransition),                  \
    x(ClientSilent),                            \
    x(Canceled),                                \
    x(ParseResponse),                           \
    x(Max)

#define RDD_NET_ERROR_ENUM(error) kError##error

enum NetError {
  RDD_NET_ERROR_GEN(RDD_NET_ERROR_ENUM)
};

#undef RDD_NET_ERROR_ENUM

extern const char* getNetErrorString(NetError error);

extern const char* getNetErrorStringByIndex(int i);

} // namespace rdd
