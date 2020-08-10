/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "raster/concurrency/CPUThreadPoolExecutor.h"
#include "raster/concurrency/IOThreadPoolExecutor.h"
#include "raster/net/Acceptor.h"
#include "raster/net/NetHub.h"

namespace raster {

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

} // namespace raster
