/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/Monitor.h"

#include "raster/thread/ThreadUtil.h"
#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "raster/util/MapUtil.h"
#include "raster/util/Time.h"

namespace rdd {

MonitorValue::MonitorValue(int type) : type_(type) {
  reset();
}

void MonitorValue::reset() {
  isset_ = type_ & (MON_CNT | MON_SUM);
  count_ = 0;
  value_ = 0;
}

void MonitorValue::add(int value) {
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

int MonitorValue::value() const {
  switch (type_) {
    case MON_CNT: return count_;
    case MON_AVG: return count_ != 0 ? value_ / count_ : 0;
    case MON_MIN:
    case MON_MAX:
    case MON_SUM: return value_;
    default: return 0;
  }
}

void Monitor::start() {
  handle_ = std::thread(&Monitor::run, this);
  handle_.detach();
}

void Monitor::run() {
  setCurrentThreadName("MonitorThread");
  open_ = true;
  CycleTimer timer(60000000); // 60s
  while (open_) {
    if (timer.isExpired()) {
      MonMap data;
      dump(data);
      if (sender_) {
        sender_->send(data);
      }
    }
    sleep(1);
  }
}

void Monitor::addToMonitor(const std::string& name, int type, int value) {
  if (!open_) {
    return;
  }
  auto key = prefix_.empty() ? name : prefix_ + '.' + name;
  std::lock_guard<std::mutex> guard(lock_);
  if (!contain(mvalues_, key)) {
    mvalues_.emplace(key, MonitorValue(type));
  }
  auto mvalue = get_ptr(mvalues_, key);
  if (mvalue->type() != type) {
    RDDLOG(ERROR) << "not same type monitor";
  }
  else {
    mvalue->add(value);
  }
}

void Monitor::dump(MonMap& data) {
  std::map<std::string, MonitorValue> mvalues;
  {
    std::lock_guard<std::mutex> guard(lock_);
    mvalues.swap(mvalues_);
  }
  for (auto& kv : mvalues) {
    if (kv.second.isSet()) {
      data[kv.first] = kv.second.value();
      kv.second.reset();
    }
  }
}

} // namespace rdd
