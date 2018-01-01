/*
 * Copyright (C) 2017, Yeolar
 */

#pragma one

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "raster/thread/SpinLock.h"
#include "raster/util/Algorithm.h"

namespace rdd {

class Degrader {
 public:
  virtual bool needDemote() = 0;
};

class CountDegrader : public Degrader {
 public:
  CountDegrader() {}

  void setup(bool open, uint32_t limit, uint32_t gap);

  bool needDemote() override;

 private:
  std::atomic<bool> open_{false};
  uint32_t limit_{0};
  uint32_t gap_{0};
  uint32_t count_{0};
  time_t ts_{0};
  SpinLock lock_;
};

class RateDegrader : public Degrader {
 public:
  RateDegrader() {}

  void setup(bool open, uint32_t limit, double rate);

  bool needDemote() override;

 private:
  std::atomic<bool> open_{false};
  double rate_{0.0};
  uint32_t limit_{0};
  uint32_t ticket_{0};
  time_t ts_{0};
  SpinLock lock_;
};

class DegraderManager {
 public:
  DegraderManager() {}

  template <class Deg, class ...Args>
  void setupDegrader(const std::string& name, Args&&... args);

  bool hit(const std::string& name);

 private:
  std::map<std::string, std::unique_ptr<Degrader>> degraders_;
  std::mutex lock_;
};

template <class Deg, class ...Args>
void DegraderManager::setupDegrader(const std::string& name, Args&&... args) {
  std::lock_guard<std::mutex> guard(lock_);
  if (!contain(degraders_, name)) {
    degraders_[name].reset(new Deg());
  }
  Degrader* d = degraders_[name].get();
  reinterpret_cast<Deg*>(d)->setup(std::forward<Args>(args)...);
}

} // namespace rdd

#define RDDDEG_HIT(name) \
  ::rdd::Singleton< ::rdd::DegraderManager>::get()->hit(name)

