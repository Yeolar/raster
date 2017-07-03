/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/net/Channel.h"
#include "rddoc/net/NetUtil.h"

namespace rdd {

class Service {
public:
  virtual ~Service() {}

  std::shared_ptr<Channel> channel() const {
    if (!channel_) {
      throw std::runtime_error("call makeChannel() first");
    }
    return channel_;
  }

  virtual void makeChannel(int port, const TimeoutOption& timeoutOpt) = 0;

protected:
  std::shared_ptr<Channel> channel_;
};

}

