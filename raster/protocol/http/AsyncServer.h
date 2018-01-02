/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Service.h"
#include "raster/protocol/http/Processor.h"

namespace rdd {

class HTTPAsyncServer : public Service {
 public:
  HTTPAsyncServer(StringPiece name) : Service(name) {}
  ~HTTPAsyncServer() override {}

  void makeChannel(int port, const TimeoutOption& timeoutOpt) override;

  void addRouter(const std::string& handler, const std::string& regex);

 private:
  std::map<std::string, std::string> routers_;
};

} // namespace rdd
