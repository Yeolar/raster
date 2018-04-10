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

#include "raster/framework/Monitor.h"

#include "accelerator/thread/ThreadUtil.h"
#include "accelerator/Algorithm.h"
#include "accelerator/Logging.h"
#include "accelerator/MapUtil.h"
#include "accelerator/Time.h"

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
  acc::setCurrentThreadName("MonitorThread");
  open_ = true;
  acc::CycleTimer timer(60000000); // 60s
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
  if (!acc::contain(mvalues_, key)) {
    mvalues_.emplace(key, MonitorValue(type));
  }
  auto mvalue = acc::get_ptr(mvalues_, key);
  if (mvalue->type() != type) {
    ACCLOG(ERROR) << "not same type monitor";
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
