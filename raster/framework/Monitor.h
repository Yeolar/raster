/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <algorithm>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "raster/util/Singleton.h"

namespace rdd {

class MonitorValue {
 public:
  enum {
    MON_CNT = 1,
    MON_AVG = 2,
    MON_MIN = 4,
    MON_MAX = 8,
    MON_SUM = 16,
  };

  explicit MonitorValue(int type);

  void reset();

  int type() const { return type_; }
  bool isSet() const { return isset_; }

  void add(int value = 0);

  int value() const;

 private:
  int type_;
  bool isset_;
  int count_;
  int64_t value_;
};

class Monitor {
 public:
  typedef std::map<std::string, int> MonMap;

  class Sender {
   public:
    virtual bool send(const MonMap& data) = 0;
  };

  Monitor() {}

  void start();

  void stop() {
    open_ = false;
  }

  void run();

  void setPrefix(const std::string& prefix) {
    prefix_ = prefix;
  }

  void setSender(std::unique_ptr<Sender>&& sender) {
    sender_ = std::move(sender);
  }

  void addToMonitor(const std::string& name, int type, int value = 0);

 private:
  void dump(MonMap& data);

  std::string prefix_;
  std::unique_ptr<Sender> sender_;
  std::map<std::string, MonitorValue> mvalues_;
  std::mutex lock_;
  std::thread handle_;
  std::atomic<bool> open_{false};
};

} // namespace rdd

#define RDDMON(name, type, value) \
  ::rdd::Singleton< ::rdd::Monitor>::get()->addToMonitor( \
    name, ::rdd::MonitorValue::MON_##type, value)
#define RDDMON_CNT(name)        RDDMON(name, CNT, 0)
#define RDDMON_AVG(name, value) RDDMON(name, AVG, value)
#define RDDMON_MIN(name, value) RDDMON(name, MIN, value)
#define RDDMON_MAX(name, value) RDDMON(name, MAX, value)
#define RDDMON_SUM(name, value) RDDMON(name, SUM, value)

