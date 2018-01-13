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

#include "raster/net/AsyncClient.h"

namespace rdd {

class BinaryAsyncClient : public AsyncClient {
 public:
  BinaryAsyncClient(const ClientOption& option);

  BinaryAsyncClient(const Peer& peer,
                    const TimeoutOption& timeout);

  BinaryAsyncClient(const Peer& peer,
                    uint64_t ctimeout = 100000,
                    uint64_t rtimeout = 1000000,
                    uint64_t wtimeout = 300000);

  ~BinaryAsyncClient() override {}

  bool recv(ByteRange& response);

  bool send(const ByteRange& request);

  bool fetch(ByteRange& response, const ByteRange& request);

 protected:
  std::shared_ptr<Channel> makeChannel() override;
};

} // namespace rdd
