/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/concurrency/CPUThreadPoolExecutor.h"
#include "raster/concurrency/IOThreadPoolExecutor.h"
#include "raster/net/Acceptor.h"
#include "raster/net/NetHub.h"

namespace rdd {

class AsyncClient;

class HubAdaptor : public NetHub {
 public:
  HubAdaptor();

  void configThreads(const std::string& name, size_t threadCount);

  void addService(std::unique_ptr<Service> service);

  void configService(
      const std::string& name,
      int port,
      const TimeoutOption& timeoutOpt);

  void startService();

  // FiberHub
  CPUThreadPoolExecutor* getCPUThreadPoolExecutor(int poolId) override;
  // NetHub
  EventLoop* getEventLoop() override;

  std::shared_ptr<CPUThreadPoolExecutor>
    getSharedCPUThreadPoolExecutor(int poolId);

 private:
  std::unique_ptr<IOThreadPoolExecutor> ioPool_;
  std::map<int, std::shared_ptr<CPUThreadPoolExecutor>> cpuPoolMap_;

  Acceptor acceptor_;
};

} // namespace rdd
