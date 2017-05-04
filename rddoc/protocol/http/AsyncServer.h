/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/net/Service.h"
#include "rddoc/protocol/http/Processor.h"
#include "rddoc/protocol/http/Protocol.h"

namespace rdd {

template <class If, class P>
class HTTPAsyncServer : public Service {
public:
  If* handler() {
    auto pf = channel_->processorFactory();
    return ((HTTPProcessorFactory<P, If, HTTPProcessor>*)(pf.get()))
      ->handler();
  }

  virtual void makeChannel(int port, const TimeoutOption& timeout_opt) {
    std::shared_ptr<Protocol> protocol(
      new HTTPProtocol());
    std::shared_ptr<ProcessorFactory> processor_factory(
      new HTTPProcessorFactory<P, If, HTTPProcessor>());
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
      peer, timeout_opt, protocol, processor_factory);
  }
};

/*
template <class If, class P>
class HTTPZlibAsyncServer : public Service {
public:
  If* handler() {
    auto pf = channel_->processorFactory();
    return ((HTTPProcessorFactory<P, If, HTTPZlibProcessor>*)(pf.get()))
      ->handler();
  }

  virtual void makeChannel(int port, const TimeoutOption& timeout_opt) {
    std::shared_ptr<Protocol> protocol(
      new HTTPZlibProtocol());
    std::shared_ptr<ProcessorFactory> processor_factory(
      new HTTPProcessorFactory<P, If, HTTPZlibProcessor>());
    Peer peer = {"", port};
    channel_ = std::make_shared<Channel>(
      peer, timeout_opt, protocol, processor_factory);
  }
};
*/

}

