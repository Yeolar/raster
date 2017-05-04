/*
 * Copyright (C) 2017, Yeolar
 */

#include <fstream>
#include "rddoc/plugins/flume/FlumeClient.h"

namespace rdd {

void FlumeClient::sendInBatch(const std::vector<LogEntry>& entries) {
  if (!client_->connected() && !client_->connect()) {
    RDDLOG(ERROR) << "connect flume client failed";
    writeToDisk(entries);
    return;
  }
  ResultCode::type r;
  if (!client_->fetch(&ScribeClient::Log, r, entries) ||
    r != ResultCode::OK) {
    RDDLOG(ERROR) << "write to flume client failed";
    writeToDisk(entries);
  }
}

void FlumeClient::writeToDisk(const std::vector<LogEntry>& entries) {
  std::string path = to<std::string>(
    logdir_, "/", category_, ".flume.", timeNowPrintf("%Y%m%d"));
  std::ofstream file(path.c_str(), std::ofstream::app);
  if (file.is_open()) {
    for (auto& entry : entries) {
      file << entry.message;
    }
    file.close();
  }
}

}

