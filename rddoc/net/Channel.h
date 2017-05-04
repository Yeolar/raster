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
          const TimeoutOption& timeout_opt,
          const std::shared_ptr<Protocol>& protocol,
          const std::shared_ptr<ProcessorFactory>& processor_factory
            = std::shared_ptr<ProcessorFactory>())
    : id_(peer.port)
    , peer_(peer)
    , timeout_opt_(timeout_opt)
    , protocol_(protocol)
    , processor_factory_(processor_factory) {
  }

  std::string str() const {
    return to<std::string>("channel[", id_, "]");
  }

  int id() const { return id_; }

  Peer peer() const { return peer_; }

  TimeoutOption timeoutOption() const { return timeout_opt_; }

  std::shared_ptr<Protocol> protocol() const {
    return protocol_;
  }
  std::shared_ptr<ProcessorFactory> processorFactory() const {
    return processor_factory_;
  }

private:
  int id_;
  Peer peer_;
  TimeoutOption timeout_opt_;
  std::shared_ptr<Protocol> protocol_;
  std::shared_ptr<ProcessorFactory> processor_factory_;
};

}

