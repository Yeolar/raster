/*
 * Copyright (C) 2017, Yeolar
 */

#pragma one

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "raster/util/MapUtil.h"
#include "raster/util/Time.h"

namespace rdd {

class Degrader {
public:
  virtual bool needDemote() = 0;
};

class LimitDegrader : public Degrader {
public:
  LimitDegrader() {}

  void setup(uint32_t limit, uint32_t gap) {
    RDDLOG(INFO) << "Degrader: setup limit=" << limit << ", gap=" << gap;
    limit_ = limit;
    gap_ = gap;
  }

  virtual bool needDemote() {
    uint32_t limit = limit_;
    uint32_t gap = gap_;
    if (limit && gap) {
      time_t ts = time(nullptr) / gap;
      if (ts != ts_) {
        ts_ = ts;
        count_ = 0;
      }
      return ++count_ > limit;
    }
    return false;
  }

private:
  std::atomic<uint32_t> limit_{0};
  std::atomic<uint32_t> gap_{0};
  std::atomic<uint32_t> count_{0};
  std::atomic<time_t> ts_{0};
};

class DegraderManager {
public:
  DegraderManager() {}

  template <class Deg, class... Args>
  void setupDegrader(const std::string& name, Args&&... args) {
    std::lock_guard<std::mutex> guard(lock_);
    if (!contain(degraders_, name)) {
      degraders_[name].reset(new Deg());
    }
    Degrader* d = degraders_[name].get();
    DCHECK(typeid(*d) == typeid(Deg));
    ((Deg*)d)->setup(std::forward<Args>(args)...);
  }

  bool hit(const std::string& name) {
    std::lock_guard<std::mutex> guard(lock_);
    auto degrader = get_ptr(degraders_, name);
    return degrader ? degrader->get()->needDemote() : false;
  }

private:
  std::map<std::string, std::unique_ptr<Degrader>> degraders_;
  std::mutex lock_;
};

} // namespace rdd

#define RDDDEG_HIT(name) \
  ::rdd::Singleton< ::rdd::DegraderManager>::get()->hit(name)

