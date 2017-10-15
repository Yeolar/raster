/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Random.h"

namespace rdd {

ThreadLocal<std::default_random_engine> Random::rng;

} // namespace rdd
