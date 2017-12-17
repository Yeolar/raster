/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/event/Event.h"
#include "raster/protocol/http/HTTPRequest.h"
#include "raster/protocol/http/HTTPResponse.h"

namespace rdd {

class HTTPEvent : public Event {
public:
  enum State {
    INIT,
    ON_HEADERS,
    ON_BODY,
    FINISH,
    ERROR,
  };

  HTTPEvent(const std::shared_ptr<Channel>& channel,
            const std::shared_ptr<Socket>& socket);

  virtual ~HTTPEvent() {}

  virtual void reset();

  void onHeaders();

  void onBody();

  State state() const { return state_; }

  HTTPRequest* request() const { return request_.get(); }
  HTTPResponse* response() const { return response_.get(); }

private:
  State state_;
  size_t headerSize_;
  std::shared_ptr<HTTPHeaders> headers_;
  std::shared_ptr<HTTPRequest> request_;
  std::shared_ptr<HTTPResponse> response_;
};

} // namespace rdd
