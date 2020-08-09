/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "raster/net/NetUtil.h"
#include "raster/protocol/binary/SyncTransport.h"

namespace raster {

class BinarySyncClient {
 public:
  BinarySyncClient(const ClientOption& option);

  BinarySyncClient(const Peer& peer,
                   const TimeoutOption& timeout);

  BinarySyncClient(const Peer& peer,
                   uint64_t ctimeout = 100000,
                   uint64_t rtimeout = 1000000,
                   uint64_t wtimeout = 300000);

  virtual ~BinarySyncClient();

  void close();

  bool connect();

  bool connected() const;

  bool fetch(acc::ByteRange& response, const acc::ByteRange& request);

 private:
  void init();

  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<BinarySyncTransport> transport_;
};

} // namespace raster
