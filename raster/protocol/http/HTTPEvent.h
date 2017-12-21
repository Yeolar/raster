/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/event/Event.h"
#include "raster/protocol/http/Transport.h"

namespace rdd {

class HTTPEvent : public Event {
public:
  HTTPEvent(const std::shared_ptr<Channel>& channel,
            const std::shared_ptr<Socket>& socket)
    : Event(channel, socket),
      transport(TransportDirection::DOWNSTREAM) {
  }

  virtual ~HTTPEvent() {}

  void reset() {
    Event::reset();
    transport.setState(HTTPTransport::kInit);
  }

  HTTPTransport transport;
};

} // namespace rdd
