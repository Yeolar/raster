/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/AtomicPtr.h"

#include <atomic>
#include "raster/util/Benchmark.h"

using namespace rdd;

class StdAtomicPtr {
private:
  std::atomic<void*> rep_;
public:
  StdAtomicPtr() { }
  explicit StdAtomicPtr(void* v) : rep_(v) { }
  inline void* Acquire_Load() const {
    return rep_.load(std::memory_order_acquire);
  }
  inline void Release_Store(void* v) {
    rep_.store(v, std::memory_order_release);
  }
  inline void* NoBarrier_Load() const {
    return rep_.load(std::memory_order_relaxed);
  }
  inline void NoBarrier_Store(void* v) {
    rep_.store(v, std::memory_order_relaxed);
  }
};

// sudo nice -n -20 ./raster/util/test/raster_util_AtomicPtrBenchmark -bm_min_iters 1000000
// ============================================================================
// AtomicPtrBenchmark.cpp                          relative  time/iter  iters/s
// ============================================================================
// std_atomic_pointer_nobarrier                                 2.71ns  368.40M
// memory_barrier_atomic_pointer_nobarrier          243.75%     1.11ns  897.97M
// std_atomic_pointer                                           2.71ns  368.91M
// memory_barrier_atomic_pointer                    243.41%     1.11ns  897.96M
// ============================================================================

BENCHMARK(std_atomic_pointer_nobarrier, n) {
  for (unsigned i = 0; i < n; ++i) {
    StdAtomicPtr ap;
    void* p = ap.NoBarrier_Load();
    rdd::doNotOptimizeAway(p);
    ap.NoBarrier_Store(p);
  }
}

BENCHMARK_RELATIVE(memory_barrier_atomic_pointer_nobarrier, n) {
  for (unsigned i = 0; i < n; ++i) {
    AtomicPtr ap;
    void* p = ap.NoBarrier_Load();
    rdd::doNotOptimizeAway(p);
    ap.NoBarrier_Store(p);
  }
}

BENCHMARK(std_atomic_pointer, n) {
  for (unsigned i = 0; i < n; ++i) {
    StdAtomicPtr ap;
    void* p = ap.Acquire_Load();
    rdd::doNotOptimizeAway(p);
    ap.Release_Store(p);
  }
}

BENCHMARK_RELATIVE(memory_barrier_atomic_pointer, n) {
  for (unsigned i = 0; i < n; ++i) {
    AtomicPtr ap;
    void* p = ap.Acquire_Load();
    rdd::doNotOptimizeAway(p);
    ap.Release_Store(p);
  }
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  rdd::runBenchmarks();
  return 0;
}
