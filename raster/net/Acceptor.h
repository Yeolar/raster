/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/event/EventLoop.h"
#include "raster/net/NetHub.h"
#include "raster/net/Service.h"

namespace rdd {

class Acceptor {
 public:
  Acceptor(std::shared_ptr<NetHub> hub);

  void addService(std::unique_ptr<Service> service);

  void configService(
      const std::string& name,
      int port,
      const TimeoutOption& timeout);

  void start();
  void stop();

 private:
  void listen(Service* service, int backlog = 64);

  std::shared_ptr<NetHub> hub_;
  std::unique_ptr<EventLoop> loop_;
  std::map<std::string, std::unique_ptr<Service>> services_;
};

} // namespace rdd
