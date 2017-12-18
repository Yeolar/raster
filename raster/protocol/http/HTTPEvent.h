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
    ON_READING_HEADERS,
    ON_READING_BODY,
    ON_READING_FINISH,
    ON_WRITING,
    ON_WRITING_FINISH,
    ERROR,
  };

  HTTPEvent(const std::shared_ptr<Channel>& channel,
            const std::shared_ptr<Socket>& socket);

  virtual ~HTTPEvent() {}

  virtual void reset();

  void onReadingHeaders();

  void onReadingBody();

  void onWritingFinish();

  State state() const { return state_; }

  HTTPRequest* request() const { return request_.get(); }
  HTTPResponse* response() const { return response_.get(); }

private:
  State state_;
  size_t headerSize_;
  std::shared_ptr<HTTPRequest> request_;
  std::shared_ptr<HTTPResponse> response_;
};

} // namespace rdd
