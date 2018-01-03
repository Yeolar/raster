/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Channel.h"
#include "raster/net/NetUtil.h"

namespace rdd {

class Service {
 public:
  Service(StringPiece name) : name_(name.str()) {}

  virtual ~Service() {}

  std::string name() const { return name_; }

  std::shared_ptr<Channel> channel() const {
    if (!channel_) {
      throw std::runtime_error("call makeChannel() first");
    }
    return channel_;
  }

  virtual void makeChannel(int port, const TimeoutOption& timeout) = 0;

 protected:
  std::string name_;
  std::shared_ptr<Channel> channel_;
};

} // namespace rdd
