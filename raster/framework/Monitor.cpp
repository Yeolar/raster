/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/Monitor.h"
#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "raster/util/MapUtil.h"

namespace rdd {

void Monitor::run() {
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
  std::lock_guard<std::mutex> guard(lock_);
  for (auto& kv : mvalues_) {
    if (kv.second.isSet()) {
      data[kv.first] = kv.second.value();
      kv.second.reset();
    }
  }
}

} // namespace rdd
