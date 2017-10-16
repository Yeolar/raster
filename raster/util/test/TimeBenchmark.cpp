/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Time.h"

#include <sys/time.h>
#include "raster/util/Benchmark.h"

using namespace rdd;

inline uint64_t systemNanoTimestampNow() {
  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  return tv.tv_sec * 1000000000 + tv.tv_nsec;
}

inline uint64_t systemTimestampNow() {
  timespec tv;
  clock_gettime(CLOCK_REALTIME, &tv);
  return tv.tv_sec * 1000000 + tv.tv_nsec / 1000;
}

BENCHMARK(systemNano, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = systemNanoTimestampNow();
    rdd::doNotOptimizeAway(t);
  }
}

BENCHMARK_RELATIVE(chronoNano, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = nanoTimestampNow();
    rdd::doNotOptimizeAway(t);
  }
}

BENCHMARK_DRAW_LINE();

BENCHMARK(systemMicro, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = systemTimestampNow();
    rdd::doNotOptimizeAway(t);
  }
}

BENCHMARK_RELATIVE(chronoMicro, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = timestampNow();
    rdd::doNotOptimizeAway(t);
  }
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  rdd::runBenchmarks();
  return 0;
}
