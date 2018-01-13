/*
 * Copyright 2017 Facebook, Inc.
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

#pragma once

#include <cstdint>
#include <random>

#include "raster/thread/ThreadUtil.h"

namespace rdd {

class Random {
 public:
  static void secureRandom(void* data, size_t len);

  template <class T>
  static typename std::enable_if<
    std::is_integral<T>::value && !std::is_same<T,bool>::value,
    T>::type
  secureRandom() {
    T val;
    secureRandom(&val, sizeof(val));
    return val;
  }

  static uint32_t rand32() {
    return std::uniform_int_distribution<uint32_t>()(*rng);
  }

  static uint32_t rand32(uint32_t min, uint32_t max) {
    if (min == max) {
      return 0;
    }
    return std::uniform_int_distribution<uint32_t>(min, max - 1)(*rng);
  }

  static uint64_t rand64() {
    return std::uniform_int_distribution<uint64_t>()(*rng);
  }

  static uint64_t rand64(uint64_t min, uint64_t max) {
    if (min == max) {
      return 0;
    }
    return std::uniform_int_distribution<uint64_t>(min, max - 1)(*rng);
  }

  static double randDouble01() {
    return std::generate_canonical<double, std::numeric_limits<double>::digits>(
        *rng);
  }

  static double randDouble(double min, double max) {
    if (std::fabs(max - min) < std::numeric_limits<double>::epsilon()) {
      return 0;
    }
    return std::uniform_real_distribution<double>(min, max)(*rng);
  }

  static ThreadLocal<std::default_random_engine> rng;
};

} // namespace rdd
