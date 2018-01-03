/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "raster/plugins/flume/gen-cpp/scribe_types.h"
#include "raster/protocol/thrift/SyncClient.h"
#include "raster/thread/AtomicLinkedList.h"

namespace rdd {

class ScribeClient;
class LogEntry;

class FlumeClient {
 public:
  FlumeClient(const ClientOption& option,
              const std::string& category,
              const std::string& logDir);

  ~FlumeClient();

  void add(std::string&& message);

  void send();

 private:
  void sendInBatch(const std::vector<LogEntry>& entries);
  void writeToDisk(const std::vector<LogEntry>& entries);

  std::string category_;
  std::string logDir_;
  AtomicLinkedList<LogEntry> queue_;
  std::unique_ptr<TSyncClient<ScribeClient>> client_;
};

} // namespace rdd
