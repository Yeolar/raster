/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
