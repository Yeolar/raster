/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Processor.h"
#include "raster/net/Transport.h"

namespace rdd {

class Channel {
public:
  Channel(const Peer& peer,
          const TimeoutOption& timeoutOpt,
          std::unique_ptr<TransportFactory> transportFactory = nullptr,
          std::unique_ptr<ProcessorFactory> processorFactory = nullptr)
    : id_(peer.port()),
      peer_(peer),
      timeoutOpt_(timeoutOpt),
      transportFactory_(std::move(transportFactory)),
      processorFactory_(std::move(processorFactory)) {
  }

  int id() const { return id_; }

  const Peer& peer() const { return peer_; }

  TimeoutOption timeoutOption() const { return timeoutOpt_; }

  TransportFactory* transportFactory() const {
    return transportFactory_.get();
  }

  ProcessorFactory* processorFactory() const {
    return processorFactory_.get();
  }

private:
  int id_;
  Peer peer_;
  TimeoutOption timeoutOpt_;
  std::unique_ptr<TransportFactory> transportFactory_;
  std::unique_ptr<ProcessorFactory> processorFactory_;
};

inline std::ostream& operator<<(std::ostream& os, const Channel& channel) {
  os << "channel[" << channel.id() << "]";
  return os;
}

} // namespace rdd
