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
#include "raster/util/Random.h"

namespace rdd {

class Sampler {
public:
  virtual bool hit() = 0;
};

class PercentSampler : public Sampler {
public:
  PercentSampler() {}

  void setup(bool open, double percent) {
    RDDLOG(INFO) << "Sampler: setup "
      << "open=" << open << ", percent=" << percent;
    open_ = open;
    percent_ = percent;
  }

  virtual bool hit() {
    bool open = open_;
    double percent = percent_;
    return open && percent > Random::randDouble01();
  }

private:
  std::atomic<bool> open_{false};
  std::atomic<double> percent_{0};
};

class SamplerManager {
public:
  SamplerManager() {}

  template <class Sam, class... Args>
  void setupSampler(const std::string& name, Args&&... args) {
    std::lock_guard<std::mutex> guard(lock_);
    if (!contain(samplers_, name)) {
      samplers_[name].reset(new Sam());
    }
    Sampler* d = samplers_[name].get();
    DCHECK(typeid(*d) == typeid(Sam));
    ((Sam*)d)->setup(std::forward<Args>(args)...);
  }

  bool hit(const std::string& name) {
    std::lock_guard<std::mutex> guard(lock_);
    auto sampler = get_ptr(samplers_, name);
    return sampler ? sampler->get()->hit() : false;
  }

private:
  std::map<std::string, std::unique_ptr<Sampler>> samplers_;
  std::mutex lock_;
};

} // namespace rdd

#define RDDSAM_HIT(name) \
  ::rdd::Singleton< ::rdd::SamplerManager>::get()->hit(name)

