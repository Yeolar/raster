/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "raster/protocol/thrift/SyncClient.h"
#include "raster/util/Conv.h"
#include "raster/util/Logging.h"
#include "raster/util/ProducerConsumerQueue.h"

namespace rdd {

class ScribeClient;
class LogEntry;

class FlumeClient {
public:
  FlumeClient(const ClientOption& option,
              const std::string& category,
              const std::string& logDir);

  ~FlumeClient();

  bool add(const std::string& message);

  void send();

private:
  void sendInBatch(const std::vector<LogEntry>& entries);
  void writeToDisk(const std::vector<LogEntry>& entries);

  std::string category_;
  std::string logDir_;
  ProducerConsumerQueue<LogEntry> queue_;
  std::shared_ptr<TSyncClient<ScribeClient>> client_;
};

} // namespace rdd
