/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/Degrader.h"

#include "raster/util/Logging.h"
#include "raster/util/MapUtil.h"
#include "raster/util/Time.h"

namespace rdd {

void CountDegrader::setup(bool open, uint32_t limit, uint32_t gap) {
  RDDLOG(INFO) << "Degrader: setup "
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
  RDDLOG(INFO) << "Degrader: setup "
    << "open=" << open << ", limit=" << limit << ", rate=" << rate;
  SpinLockGuard guard(lock_);
  open_ = open;
  rate_ = rate;
  limit_ = limit;
  ticket_ = limit;
}

bool RateDegrader::needDemote() {
  if (open_) {
    uint64_t ts = timestampNow();
    SpinLockGuard guard(lock_);
    if (ts_ != 0) {
      uint32_t incr = std::max(ts - ts_, 0ul) * rate_;
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
  auto degrader = get_ptr(degraders_, name);
  return degrader ? degrader->get()->needDemote() : false;
}

} // namespace rdd
