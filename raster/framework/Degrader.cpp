/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/framework/Degrader.h"

#include "accelerator/Logging.h"
#include "accelerator/MapUtil.h"
#include "accelerator/Time.h"

namespace raster {

using acc::SpinLockGuard;

void CountDegrader::setup(bool open, uint32_t limit, uint32_t gap) {
  ACCLOG(INFO) << "Degrader: setup "
    << "open=" << open << ", limit=" << limit << ", gap=" << gap;
  SpinLockGuard guard(lock_);
  open_ = open;
  limit_ = limit;
  gap_ = gap;
}

bool CountDegrader::needDemote() {
  if (open_) {
    time_t ts = time(nullptr);
    SpinLockGuard guard(lock_);
    ts /= gap_;
    if (ts != ts_) {
      ts_ = ts;
      count_ = 0;
    }
    return ++count_ > limit_;
  }
  return false;
}

void RateDegrader::setup(bool open, uint32_t limit, double rate) {
  ACCLOG(INFO) << "Degrader: setup "
    << "open=" << open << ", limit=" << limit << ", rate=" << rate;
  SpinLockGuard guard(lock_);
  open_ = open;
  rate_ = rate;
  limit_ = limit;
  ticket_ = limit;
}

bool RateDegrader::needDemote() {
  if (open_) {
    uint64_t ts = acc::timestampNow();
    SpinLockGuard guard(lock_);
    if (ts_ != 0) {
      uint32_t incr = std::max(ts - ts_, uint64_t(0)) * rate_;
      ticket_ = std::min(ticket_ + incr, limit_);
    }
    ts_ = ts;
    if (ticket_ == 0) {
      return true;
    }
    ticket_--;
  }
  return false;
}

bool DegraderManager::hit(const std::string& name) {
  std::lock_guard<std::mutex> guard(lock_);
  auto degrader = acc::get_ptr(degraders_, name);
  return degrader ? degrader->get()->needDemote() : false;
}

} // namespace raster
