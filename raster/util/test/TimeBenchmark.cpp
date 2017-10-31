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

// sudo nice -n -20 ./raster/util/test/raster_util_TimeBenchmark -bm_min_iters 1000000
// ============================================================================
// TimeBenchmark.cpp                               relative  time/iter  iters/s
// ============================================================================
// system_nano                                                 16.15ns   61.92M
// chrono_nano                                       95.08%    16.99ns   58.87M
// ----------------------------------------------------------------------------
// system_micro                                                17.26ns   57.92M
// chrono_micro                                      96.88%    17.82ns   56.11M
// ----------------------------------------------------------------------------
// time                                                         2.51ns  399.09M
// ============================================================================

BENCHMARK(system_nano, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = systemNanoTimestampNow();
    rdd::doNotOptimizeAway(t);
  }
}

BENCHMARK_RELATIVE(chrono_nano, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = nanoTimestampNow();
    rdd::doNotOptimizeAway(t);
  }
}

BENCHMARK_DRAW_LINE();

BENCHMARK(system_micro, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = systemTimestampNow();
    rdd::doNotOptimizeAway(t);
  }
}

BENCHMARK_RELATIVE(chrono_micro, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = timestampNow();
    rdd::doNotOptimizeAway(t);
  }
}

BENCHMARK_DRAW_LINE();

BENCHMARK(time, n) {
  for (unsigned i = 0; i < n; ++i) {
    auto t = time(nullptr);
    rdd::doNotOptimizeAway(t);
  }
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  rdd::runBenchmarks();
  return 0;
}
