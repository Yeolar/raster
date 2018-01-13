/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
