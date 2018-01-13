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

#include "raster/net/Processor.h"
#include "raster/net/Transport.h"

namespace rdd {

class Channel {
 public:
  Channel(const Peer& peer,
          const TimeoutOption& timeout,
          std::unique_ptr<TransportFactory> transportFactory = nullptr,
          std::unique_ptr<ProcessorFactory> processorFactory = nullptr)
    : id_(peer.port()),
      peer_(peer),
      timeout_(timeout),
      transportFactory_(std::move(transportFactory)),
      processorFactory_(std::move(processorFactory)) {
  }

  int id() const { return id_; }

  const Peer& peer() const { return peer_; }

  TimeoutOption timeoutOption() const { return timeout_; }

  TransportFactory* transportFactory() const {
    return transportFactory_.get();
  }

  ProcessorFactory* processorFactory() const {
    return processorFactory_.get();
  }

 private:
  int id_;
  Peer peer_;
  TimeoutOption timeout_;
  std::unique_ptr<TransportFactory> transportFactory_;
  std::unique_ptr<ProcessorFactory> processorFactory_;
};

inline std::ostream& operator<<(std::ostream& os, const Channel& channel) {
  os << "channel[" << channel.id() << "]";
  return os;
}

} // namespace rdd
