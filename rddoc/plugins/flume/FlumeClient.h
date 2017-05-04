/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "rddoc/plugins/flume/gen-cpp/Scribe.h"
#include "rddoc/plugins/flume/gen-cpp/scribe_types.h"
#include "rddoc/protocol/thrift/SyncClient.h"
#include "rddoc/util/Conv.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/ProducerConsumerVector.h"

namespace rdd {

class FlumeClient {
public:
  FlumeClient(const ClientOption& option,
         const std::string& category,
         const std::string& logdir = "/opt/logs/logs/flume_delay")
    : category_(category), logdir_(logdir) {
    client_ = std::make_shared<TSyncClient<ScribeClient>>(option);
    if (!client_->connect()) {
      RDDLOG(ERROR) << "connect flume client failed";
    }
  }

  ~FlumeClient() {
    send();
    send();
  }

  void add(const std::string& message) {
    LogEntry entry;
    entry.category = category_;
    entry.message = message + "\n";
    pcvec_.add(entry);
  }

  void send() {
    using namespace std::placeholders;
    pcvec_.consume(std::bind(&FlumeClient::sendInBatch, this, _1));
  }

  size_t bufferSize() const { return pcvec_.size(); }
  size_t bufferCapacity() const { return pcvec_.capacity(); }

private:
  void sendInBatch(const std::vector<LogEntry>& entries);
  void writeToDisk(const std::vector<LogEntry>& entries);

  std::string category_;
  std::string logdir_;
  std::shared_ptr<TSyncClient<ScribeClient>> client_;
  ProducerConsumerVector<LogEntry> pcvec_;
};

}
