/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstdint>
#include <random>
#include "raster/util/ThreadUtil.h"

namespace rdd {

class Random {
public:
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
