/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Processor.h"
#include "raster/net/Protocol.h"

namespace rdd {

class Channel {
public:
  enum {
    DEFAULT,
    HTTP,
  };

  Channel(int type,
          const Peer& peer,
          const TimeoutOption& timeoutOpt,
          const std::shared_ptr<Protocol>& protocol,
          const std::shared_ptr<ProcessorFactory>& processorFactory
            = std::shared_ptr<ProcessorFactory>())
    : type_(type)
    , id_(peer.port)
    , peer_(peer)
    , timeoutOpt_(timeoutOpt)
    , protocol_(protocol)
    , processorFactory_(processorFactory) {
  }

  std::string str() const {
    return to<std::string>("channel[", id_, "]");
  }

  int type() const { return type_; }

  int id() const { return id_; }

  Peer peer() const { return peer_; }

  TimeoutOption timeoutOption() const { return timeoutOpt_; }

  std::shared_ptr<Protocol> protocol() const {
    return protocol_;
  }
  std::shared_ptr<ProcessorFactory> processorFactory() const {
    return processorFactory_;
  }

private:
  int type_;
  int id_;
  Peer peer_;
  TimeoutOption timeoutOpt_;
  std::shared_ptr<Protocol> protocol_;
  std::shared_ptr<ProcessorFactory> processorFactory_;
};

} // namespace rdd
