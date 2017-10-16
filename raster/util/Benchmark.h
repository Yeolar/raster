/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string.h>
#include <cassert>
#include <functional>
#include <limits>
#include <type_traits>
#include <boost/function_types/function_arity.hpp>
#include <gflags/gflags.h>
#include "raster/util/Macro.h"
#include "raster/util/ScopeGuard.h"
#include "raster/util/Time.h"
#include "raster/util/Traits.h"

#ifndef __FILENAME__
#define __FILENAME__ ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#endif

namespace rdd {

typedef std::function<
  std::pair<uint64_t, unsigned int>(unsigned int)> BenchmarkFun;

/**
 * Runs all benchmarks defined. Usually put in main().
 */
void runBenchmarks();

namespace detail {

void addBenchmarkImpl(const char* file, const char* name, BenchmarkFun);

} // namespace detail

/**
 * Supporting type for BENCHMARK_SUSPEND defined below.
 */
struct BenchmarkSuspender {
  BenchmarkSuspender() {
    start = nanoTimestampNow();
  }

  NOCOPY(BenchmarkSuspender);

  BenchmarkSuspender(BenchmarkSuspender && rhs) noexcept {
    start = rhs.start;
    rhs.start = 0;
  }

  BenchmarkSuspender& operator=(BenchmarkSuspender && rhs) {
    if (start != 0) {
      tally();
    }
    start = rhs.start;
    rhs.start = 0;
    return *this;
  }

  ~BenchmarkSuspender() {
    if (start != 0) {
      tally();
    }
  }

  void dismiss() {
    assert(start != 0);
    tally();
    start = 0;
  }

  void rehire() {
    assert(start == 0);
    start = nanoTimestampNow();
  }

  template <class F>
  auto dismissing(F f) -> typename std::result_of<F()>::type {
    SCOPE_EXIT { rehire(); };
    dismiss();
    return f();
  }

  explicit operator bool() const {
    return false;
  }

  static uint64_t timeSpent;

 private:
  void tally() {
    auto end = nanoTimestampNow();
    timeSpent += end - start;
    start = end;
  }

  uint64_t start;
};

/**
 * Adds a benchmark. Usually not called directly but instead through
 * the macro BENCHMARK defined below. The lambda function involved
 * must take exactly one parameter of type unsigned, and the benchmark
 * uses it with counter semantics (iteration occurs inside the
 * function).
 */
template <typename Lambda>
typename std::enable_if<
  boost::function_types::function_arity<decltype(&Lambda::operator())>::value
  == 2
>::type
addBenchmark(const char* file, const char* name, Lambda&& lambda) {
  auto execute = [=](unsigned int times) {
    BenchmarkSuspender::timeSpent = {};
    unsigned int niter;

    // CORE MEASUREMENT STARTS
    uint64_t start = nanoTimestampNow();
    niter = lambda(times);
    uint64_t end = nanoTimestampNow();
    // CORE MEASUREMENT ENDS

    return std::pair<uint64_t, unsigned int>(
        (end - start) - BenchmarkSuspender::timeSpent, niter);
  };

  detail::addBenchmarkImpl(file, name, BenchmarkFun(execute));
}

/**
 * Adds a benchmark. Usually not called directly but instead through
 * the macro BENCHMARK defined below. The lambda function involved
 * must take zero parameters, and the benchmark calls it repeatedly
 * (iteration occurs outside the function).
 */
template <typename Lambda>
typename std::enable_if<
  boost::function_types::function_arity<decltype(&Lambda::operator())>::value
  == 1
>::type
addBenchmark(const char* file, const char* name, Lambda&& lambda) {
  addBenchmark(file, name, [=](unsigned int times) {
      unsigned int niter = 0;
      while (times-- > 0) {
        niter += lambda();
      }
      return niter;
    });
}

/**
 * Call doNotOptimizeAway(var) to ensure that var will be computed even
 * post-optimization.  Use it for variables that are computed during
 * benchmarking but otherwise are useless. The compiler tends to do a
 * good job at eliminating unused variables, and this function fools it
 * into thinking var is in fact needed.
 */

namespace detail {
template <typename T>
struct DoNotOptimizeAwayNeedsIndirect {
  using Decayed = typename std::decay<T>::type;

  // First two constraints ensure it can be an "r" operand.
  // std::is_pointer check is because callers seem to expect that
  // doNotOptimizeAway(&x) is equivalent to doNotOptimizeAway(x).
  constexpr static bool value = !rdd::IsTriviallyCopyable<Decayed>::value ||
      sizeof(Decayed) > sizeof(long) || std::is_pointer<Decayed>::value;
};
} // namespace detail

template <typename T>
auto doNotOptimizeAway(const T& datum) -> typename std::enable_if<
    !detail::DoNotOptimizeAwayNeedsIndirect<T>::value>::type {
  // The "r" constraint forces the compiler to make datum available
  // in a register to the asm block, which means that it must have
  // computed/loaded it.  We use this path for things that are <=
  // sizeof(long) (they have to fit), trivial (otherwise the compiler
  // doesn't want to put them in a register), and not a pointer (because
  // doNotOptimizeAway(&foo) would otherwise be a foot gun that didn't
  // necessarily compute foo).
  //
  // An earlier version of this method had a more permissive input operand
  // constraint, but that caused unnecessary variation between clang and
  // gcc benchmarks.
  asm volatile("" ::"r"(datum));
}

template <typename T>
auto doNotOptimizeAway(const T& datum) -> typename std::enable_if<
    detail::DoNotOptimizeAwayNeedsIndirect<T>::value>::type {
  // This version of doNotOptimizeAway tells the compiler that the asm
  // block will read datum from memory, and that in addition it might read
  // or write from any memory location.  If the memory clobber could be
  // separated into input and output that would be preferrable.
  asm volatile("" ::"m"(datum) : "memory");
}

} // namespace rdd

/**
 * Introduces a benchmark function. Used internally, see BENCHMARK and
 * friends below.
 */
#define BENCHMARK_IMPL(funName, stringName, rv, paramType, paramName)   \
  static void funName(paramType);                                       \
  static bool RDD_ANONYMOUS_VARIABLE(rddBenchmarkUnused) = (            \
    ::rdd::addBenchmark(__FILENAME__, stringName,                       \
      [](paramType paramName) -> unsigned { funName(paramName);         \
                                            return rv; }),              \
    true);                                                              \
  static void funName(paramType paramName)

/**
 * Introduces a benchmark function. Use with either one or two arguments.
 * The first is the name of the benchmark. Use something descriptive, such
 * as insertVectorBegin. The second argument may be missing, or could be a
 * symbolic counter. The counter dictates how many internal iteration the
 * benchmark does. Example:
 *
 * BENCHMARK(vectorPushBack) {
 *   vector<int> v;
 *   v.push_back(42);
 * }
 *
 * BENCHMARK(insertVectorBegin, n) {
 *   vector<int> v;
 *   FOR_EACH_RANGE (i, 0, n) {
 *     v.insert(v.begin(), 42);
 *   }
 * }
 */
#define BENCHMARK(name, ...)                                    \
  BENCHMARK_IMPL(                                               \
    name,                                                       \
    RDD_STRINGIZE(name),                                        \
    RDD_ARG_2_OR_1(1, ## __VA_ARGS__),                          \
    RDD_ARG_1_OR_NONE(unsigned, ## __VA_ARGS__),                \
    __VA_ARGS__)

/**
 * Defines a benchmark that passes a parameter to another one. This is
 * common for benchmarks that need a "problem size" in addition to
 * "number of iterations". Consider:
 *
 * void pushBack(uint n, size_t initialSize) {
 *   vector<int> v;
 *   BENCHMARK_SUSPEND {
 *     v.resize(initialSize);
 *   }
 *   FOR_EACH_RANGE (i, 0, n) {
 *    v.push_back(i);
 *   }
 * }
 * BENCHMARK_PARAM(pushBack, 0)
 * BENCHMARK_PARAM(pushBack, 1000)
 * BENCHMARK_PARAM(pushBack, 1000000)
 *
 * The benchmark above estimates the speed of push_back at different
 * initial sizes of the vector. The framework will pass 0, 1000, and
 * 1000000 for initialSize, and the iteration count for n.
 */
#define BENCHMARK_PARAM(name, param)                            \
  BENCHMARK_NAMED_PARAM(name, param, param)

/*
 * Like BENCHMARK_PARAM(), but allows a custom name to be specified for each
 * parameter, rather than using the parameter value.
 *
 * Useful when the parameter value is not a valid token for string pasting,
 * of when you want to specify multiple parameter arguments.
 *
 * For example:
 *
 * void addValue(uint n, int64_t bucketSize, int64_t min, int64_t max) {
 *   Histogram<int64_t> hist(bucketSize, min, max);
 *   int64_t num = min;
 *   FOR_EACH_RANGE (i, 0, n) {
 *     hist.addValue(num);
 *     ++num;
 *     if (num > max) { num = min; }
 *   }
 * }
 *
 * BENCHMARK_NAMED_PARAM(addValue, 0_to_100, 1, 0, 100)
 * BENCHMARK_NAMED_PARAM(addValue, 0_to_1000, 10, 0, 1000)
 * BENCHMARK_NAMED_PARAM(addValue, 5k_to_20k, 250, 5000, 20000)
 */
#define BENCHMARK_NAMED_PARAM(name, param_name, ...)            \
  BENCHMARK_IMPL(                                               \
      RDD_CONCATENATE(name, RDD_CONCATENATE(_, param_name)),    \
      RDD_STRINGIZE(name) "(" RDD_STRINGIZE(param_name) ")",    \
      iters,                                                    \
      unsigned,                                                 \
      iters) {                                                  \
    name(iters, ## __VA_ARGS__);                                \
  }

/**
 * Just like BENCHMARK, but prints the time relative to a
 * baseline. The baseline is the most recent BENCHMARK() seen in
 * the current scope. Example:
 *
 * // This is the baseline
 * BENCHMARK(insertVectorBegin, n) {
 *   vector<int> v;
 *   FOR_EACH_RANGE (i, 0, n) {
 *     v.insert(v.begin(), 42);
 *   }
 * }
 *
 * BENCHMARK_RELATIVE(insertListBegin, n) {
 *   list<int> s;
 *   FOR_EACH_RANGE (i, 0, n) {
 *     s.insert(s.begin(), 42);
 *   }
 * }
 *
 * Any number of relative benchmark can be associated with a
 * baseline. Another BENCHMARK() occurrence effectively establishes a
 * new baseline.
 */
#define BENCHMARK_RELATIVE(name, ...)                           \
  BENCHMARK_IMPL(                                               \
    name,                                                       \
    "%" RDD_STRINGIZE(name),                                    \
    RDD_ARG_2_OR_1(1, ## __VA_ARGS__),                          \
    RDD_ARG_1_OR_NONE(unsigned, ## __VA_ARGS__),                \
    __VA_ARGS__)

/**
 * A combination of BENCHMARK_RELATIVE and BENCHMARK_PARAM.
 */
#define BENCHMARK_RELATIVE_PARAM(name, param)                   \
  BENCHMARK_RELATIVE_NAMED_PARAM(name, param, param)

/**
 * A combination of BENCHMARK_RELATIVE and BENCHMARK_NAMED_PARAM.
 */
#define BENCHMARK_RELATIVE_NAMED_PARAM(name, param_name, ...)   \
  BENCHMARK_IMPL(                                               \
      RDD_CONCATENATE(name, RDD_CONCATENATE(_, param_name)),    \
      "%" RDD_STRINGIZE(name) "(" RDD_STRINGIZE(param_name) ")",\
      iters,                                                    \
      unsigned,                                                 \
      iters) {                                                  \
    name(iters, ## __VA_ARGS__);                                \
  }

/**
 * Draws a line of dashes.
 */
#define BENCHMARK_DRAW_LINE()                                   \
  static bool RDD_ANONYMOUS_VARIABLE(rddBenchmarkUnused) = (    \
    ::rdd::addBenchmark(__FILENAME__, "-",                      \
      []() -> unsigned { return 0; }),                          \
    true);

/**
 * Allows execution of code that doesn't count torward the benchmark's
 * time budget. Example:
 *
 * BENCHMARK_START_GROUP(insertVectorBegin, n) {
 *   vector<int> v;
 *   BENCHMARK_SUSPEND {
 *     v.reserve(n);
 *   }
 *   FOR_EACH_RANGE (i, 0, n) {
 *     v.insert(v.begin(), 42);
 *   }
 * }
 */
#define BENCHMARK_SUSPEND                                       \
  if (auto RDD_ANONYMOUS_VARIABLE(BENCHMARK_SUSPEND) =          \
      ::rdd::BenchmarkSuspender()) {}                           \
  else

