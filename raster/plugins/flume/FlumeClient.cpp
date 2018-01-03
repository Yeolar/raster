/*
 * Copyright (C) 2017, Yeolar
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
