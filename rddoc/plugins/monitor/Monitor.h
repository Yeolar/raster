/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <algorithm>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include "rddoc/util/Singleton.h"
#include "rddoc/util/ThreadUtil.h"

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

  explicit MonitorValue(int type) : type_(type) {
    reset();
  }

  void reset() {
    isset_ = type_ & (MON_CNT | MON_SUM);
    count_ = 0;
    value_ = 0;
  }

  int type() const { return type_; }
  bool isSet() const { return isset_; }

  void add(int value = 0) {
    isset_ = true;
    count_++;
    switch (type_) {
      case MON_AVG:
      case MON_SUM: value_ += value; break;
      case MON_MIN: value_ = std::min(value_, (int64_t)value); break;
      case MON_MAX: value_ = std::max(value_, (int64_t)value); break;
      default: break;
    }
  }

  int value() const {
    switch (type_) {
      case MON_CNT: return count_;
      case MON_AVG: return count_ != 0 ? value_ / count_ : 0;
      case MON_MIN:
      case MON_MAX:
      case MON_SUM: return value_;
      default: return 0;
    }
  }

private:
  int type_;
  bool isset_;
  int count_;
  int64_t value_;
};

class Monitor {
public:
  Monitor() {}

  void start() {
    handle_ = std::thread(&Monitor::run, this);
    setThreadName(handle_.native_handle(), "MonitorThread");
    handle_.detach();
  }

  void run();

  void setPrefix(const std::string& prefix) {
    prefix_ = prefix;
  }

  void addToMonitor(const std::string& name, int type, int value = 0);

private:
  void dump(std::map<std::string, int>& data);

  std::string prefix_;
  std::map<std::string, MonitorValue> mvalues_;
  std::mutex lock_;
  std::thread handle_;
  std::atomic<bool> open_{false};
};

}

#define RDDMON(name, type, value) \
  ::rdd::Singleton< ::rdd::Monitor>::get()->addToMonitor( \
    name, ::rdd::MonitorValue::MON_##type, value)
#define RDDMON_CNT(name)        RDDMON(name, CNT, 0)
#define RDDMON_AVG(name, value) RDDMON(name, AVG, value)
#define RDDMON_MIN(name, value) RDDMON(name, MIN, value)
#define RDDMON_MAX(name, value) RDDMON(name, MAX, value)
#define RDDMON_SUM(name, value) RDDMON(name, SUM, value)

