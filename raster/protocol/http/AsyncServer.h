/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <boost/regex.hpp>

#include "raster/net/Service.h"
#include "raster/protocol/http/Processor.h"

namespace rdd {

class HTTPAsyncServer : public Service {
 public:
  HTTPAsyncServer(StringPiece name) : Service(name) {}
  ~HTTPAsyncServer() override {}

  void makeChannel(int port, const TimeoutOption& timeoutOpt) override;

  template <class T, class ...Args>
  void addHandler(const std::string& regex, Args&&... args) {
    handlers_.emplace(boost::regex(regex),
                      std::make_shared<T>(std::forward<Args>(args)...));
  }

  std::shared_ptr<RequestHandler> matchHandler(const std::string& url) const;

 private:
  std::map<boost::regex, std::shared_ptr<RequestHandler>> handlers_;
};

} // namespace rdd
