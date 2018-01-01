/*
 * Copyright (C) 2017, Yeolar
 */

#pragma one

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "raster/util/Algorithm.h"

namespace rdd {

class Sampler {
 public:
  virtual bool hit() = 0;
};

class PercentSampler : public Sampler {
 public:
  PercentSampler() {}

  void setup(bool open, double percent);

  bool hit() override;

 private:
  std::atomic<bool> open_{false};
  std::atomic<double> percent_{0};
};

class SamplerManager {
 public:
  SamplerManager() {}

  template <class Sam, class ...Args>
  void setupSampler(const std::string& name, Args&&... args);

  bool hit(const std::string& name);

 private:
  std::map<std::string, std::unique_ptr<Sampler>> samplers_;
  std::mutex lock_;
};

template <class Sam, class ...Args>
void SamplerManager::setupSampler(const std::string& name, Args&&... args) {
  std::lock_guard<std::mutex> guard(lock_);
  if (!contain(samplers_, name)) {
    samplers_[name].reset(new Sam());
  }
  Sampler* d = samplers_[name].get();
  reinterpret_cast<Sam*>(d)->setup(std::forward<Args>(args)...);
}

} // namespace rdd

#define RDDSAM_HIT(name) \
  ::rdd::Singleton< ::rdd::SamplerManager>::get()->hit(name)

