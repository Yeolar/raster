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
#include "raster/net/Socket.h"
#include "raster/protocol/binary/Transport.h"

namespace raster {

class BinarySyncTransport : public BinaryTransport {
 public:
  BinarySyncTransport(const Peer& peer, const TimeoutOption& timeout)
    : BinaryTransport(),
      peer_(peer),
      timeout_(timeout) {}

  ~BinarySyncTransport() override {}

  void open();

  bool isOpen();

  void close();

  void send(const acc::ByteRange& request);

  void recv(acc::ByteRange& response);

 private:
  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<Socket> socket_;
};

} // namespace raster
