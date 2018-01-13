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
