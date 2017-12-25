/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/IOBufQueue.h"
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
  IOBufQueue readBuf_{IOBufQueue::cacheChainLength()};
  IOBufQueue writeBuf_{IOBufQueue::cacheChainLength()};
};

class TransportFactory {
public:
  virtual std::unique_ptr<Transport> create() = 0;
};

} // namespace rdd
