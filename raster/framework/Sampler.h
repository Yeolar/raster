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

#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "accelerator/Algorithm.h"

namespace raster {

class Sampler {
 public:
  virtual ~Sampler() {}

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
  if (!acc::containKey(samplers_, name)) {
    samplers_[name].reset(new Sam());
  }
  Sampler* d = samplers_[name].get();
  reinterpret_cast<Sam*>(d)->setup(std::forward<Args>(args)...);
}

} // namespace raster

#define RASTERSAM_HIT(name) \
  ::acc::Singleton< ::raster::SamplerManager>::get()->hit(name)

