/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/plugins/monitor/Monitor.h"
#include "rddoc/plugins/monitor/FalconClient.h"
#include "rddoc/util/Algorithm.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/MapUtil.h"

namespace rdd {

void Monitor::run() {
  FalconClient falcon;
  CycleTimer timer(60000000); // 60s
  while (true) {
    if (timer.isExpired()) {
      std::map<std::string, int> data;
      dump(data);
      falcon.send(data);
    }
    sleep(1);
  }
}

void Monitor::addToMonitor(const std::string& name, int type, int value) {
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

void Monitor::dump(std::map<std::string, int>& data) {
  std::lock_guard<std::mutex> guard(lock_);
  for (auto& kv : mvalues_) {
    if (kv.second.isSet()) {
      data[kv.first] = kv.second.value();
      kv.second.reset();
    }
  }
}

}

