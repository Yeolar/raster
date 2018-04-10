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

#include "accelerator/io/IOBufQueue.h"
#include "raster/net/Socket.h"

namespace rdd {

class Transport {
 public:
  enum IngressState {
    kInit,
    kOnReading,
    kFinish,
    kError,
  };

  static const uint32_t kMinReadSize = 1460;
  static const uint32_t kMaxReadSize = 4000;

  virtual ~Transport() {}

  virtual void reset() = 0;

  virtual void processReadData() = 0;

  void setPeerAddress(const Peer& peer) { peerAddr_ = peer; }
  const Peer& peerAddress() const { return peerAddr_; }

  void setLocalAddress(const Peer& local) { localAddr_ = local; }
  const Peer& localAddress() const { return localAddr_; }

  void getReadBuffer(void** buf, size_t* bufSize);
  void readDataAvailable(size_t readSize);

  int readData(Socket* socket);
  int writeData(Socket* socket);

  void clone(Transport* other);

 protected:
  Peer peerAddr_;
  Peer localAddr_;
  IngressState state_;
  acc::IOBufQueue readBuf_{acc::IOBufQueue::cacheChainLength()};
  acc::IOBufQueue writeBuf_{acc::IOBufQueue::cacheChainLength()};
};

class TransportFactory {
 public:
  virtual ~TransportFactory() {}
  virtual std::unique_ptr<Transport> create() = 0;
};

} // namespace rdd
