/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/Sampler.h"

#include "raster/util/Logging.h"
#include "raster/util/MapUtil.h"
#include "raster/util/Random.h"

namespace rdd {

void PercentSampler::setup(bool open, double percent) {
  RDDLOG(INFO) << "Sampler: setup "
    << "open=" << open << ", percent=" << percent;
  open_ = open;
  percent_ = percent;
}

bool PercentSampler::hit() {
  bool open = open_;
  double percent = percent_;
  return open && percent > Random::randDouble01();
}

bool SamplerManager::hit(const std::string& name) {
  std::lock_guard<std::mutex> guard(lock_);
  auto sampler = get_ptr(samplers_, name);
  return sampler ? sampler->get()->hit() : false;
}

} // namespace rdd
