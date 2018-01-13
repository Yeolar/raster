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

#include "raster/plugins/flume/FlumeClient.h"

#include <fstream>

#include "raster/plugins/flume/gen-cpp/Scribe.h"
#include "raster/util/Time.h"

namespace rdd {

FlumeClient::FlumeClient(const ClientOption& option,
                         const std::string& category,
                         const std::string& logDir)
  : category_(category), logDir_(logDir) {
  client_ = make_unique<TSyncClient<ScribeClient>>(option);
}

FlumeClient::~FlumeClient() {
  send();
}

void FlumeClient::add(std::string&& message) {
  LogEntry entry;
  entry.category = category_;
  entry.message = std::move(message);
  entry.message += "\n";
  queue_.insertHead(std::move(entry));
}

void FlumeClient::send() {
  std::vector<LogEntry> entries;
  queue_.sweep([&](LogEntry&& entry) {
    entries.push_back(std::move(entry));
  });
  sendInBatch(entries);
}

void FlumeClient::sendInBatch(const std::vector<LogEntry>& entries) {
  if (!client_->connected() && !client_->connect()) {
    RDDLOG(ERROR) << "connect flume client failed";
    writeToDisk(entries);
    return;
  }

  ResultCode r;
  if (!client_->fetch(&ScribeClient::Log, r, entries) ||
      r != ResultCode::OK) {
    RDDLOG(ERROR) << "write to flume client failed";
    writeToDisk(entries);
  }
}

void FlumeClient::writeToDisk(const std::vector<LogEntry>& entries) {
  std::string path = to<std::string>(
    logDir_, "/", category_, ".flume.", timeNowPrintf("%Y%m%d"));
  std::ofstream file(path.c_str(), std::ofstream::app);

  if (file.is_open()) {
    for (auto& entry : entries) {
      file << entry.message;
    }
    file.close();
  }
}

} // namespace rdd
