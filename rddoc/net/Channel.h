/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/net/Processor.h"
#include "rddoc/net/Protocol.h"

namespace rdd {

class Channel {
public:
  Channel(const Peer& peer,
          const TimeoutOption& timeoutOpt,
          const std::shared_ptr<Protocol>& protocol,
          const std::shared_ptr<ProcessorFactory>& processorFactory
            = std::shared_ptr<ProcessorFactory>())
    : id_(peer.port)
    , peer_(peer)
    , timeoutOpt_(timeoutOpt)
    , protocol_(protocol)
    , processorFactory_(processorFactory) {
  }

  std::string str() const {
    return to<std::string>("channel[", id_, "]");
  }

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
  int id_;
  Peer peer_;
  TimeoutOption timeoutOpt_;
  std::shared_ptr<Protocol> protocol_;
  std::shared_ptr<ProcessorFactory> processorFactory_;
};

}

