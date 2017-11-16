/*
 * Copyright (C) 2017, Yeolar
 */

#include <fstream>

#include "raster/plugins/flume/FlumeClient.h"
#include "raster/plugins/flume/gen-cpp/Scribe.h"
#include "raster/plugins/flume/gen-cpp/scribe_types.h"

namespace rdd {

FlumeClient::FlumeClient(const ClientOption& option,
                         const std::string& category,
                         const std::string& logDir)
  : category_(category), logDir_(logDir), queue_(10240) {
  client_ = std::make_shared<TSyncClient<ScribeClient>>(option);
  if (!client_->connect()) {
    RDDLOG(ERROR) << "connect flume client failed";
  }
}

FlumeClient::~FlumeClient() {
  send();
}

bool FlumeClient::add(const std::string& message) {
  if (queue_.isFull()) {
    return false;
  }
  LogEntry entry;
  entry.category = category_;
  entry.message = message + "\n";
  queue_.write(entry);
  return true;
}

void FlumeClient::send() {
  std::vector<LogEntry> entries;
  while (!queue_.isEmpty()) {
    LogEntry entry;
    queue_.read(entry);
    entries.push_back(entry);
  }
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
