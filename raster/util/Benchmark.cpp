/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Benchmark.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>
#include "raster/util/Logging.h"

using namespace std;

DEFINE_int32(
    bm_min_iters,
    1,
    "Minimum # of iterations we'll try for each benchmark.");

DEFINE_int64(
    bm_max_iters,
    1 << 30,
    "Maximum # of iterations we'll try for each benchmark.");

DEFINE_int32(
    bm_max_secs,
    1,
    "Maximum # of seconds we'll spend on each benchmark.");

namespace rdd {

uint64_t BenchmarkSuspender::timeSpent;

vector<tuple<string, string, BenchmarkFun>>& benchmarks() {
  static vector<tuple<string, string, BenchmarkFun>> _benchmarks;
  return _benchmarks;
}

#define RDD_GLOBAL_BENCHMARK_BASELINE rddGlobalBenchmarkBaseline

// Add the global baseline
BENCHMARK(RDD_GLOBAL_BENCHMARK_BASELINE) {
  asm volatile("");
}

size_t getGlobalBenchmarkBaselineIndex() {
  const char *global = RDD_STRINGIZE2(RDD_GLOBAL_BENCHMARK_BASELINE);
  auto it = std::find_if(
    benchmarks().begin(),
    benchmarks().end(),
    [global](const tuple<string, string, BenchmarkFun> &v) {
      return get<1>(v) == global;
    }
  );
  RDDCHECK(it != benchmarks().end());
  return size_t(std::distance(benchmarks().begin(), it));
}

#undef RDD_GLOBAL_BENCHMARK_BASELINE

void detail::addBenchmarkImpl(const char* file, const char* name,
                              BenchmarkFun fun) {
  benchmarks().emplace_back(file, name, std::move(fun));
}

static double runBenchmarkGetNSPerIteration(const BenchmarkFun& fun,
                                            const double globalBaseline) {
  // We choose a minimum minimum (sic) of 100,000 nanoseconds.
  static const uint64_t minNanoseconds = 100000;

  // We do measurements in several epochs and take the minimum, to
  // account for jitter.
  static const unsigned int epochs = 1000;
  // We establish a total time budget as we don't want a measurement
  // to take too long. This will curtail the number of actual epochs.
  const uint64_t timeBudget = FLAGS_bm_max_secs * 1000000000;
  uint64_t global = nanoTimestampNow();

  double epochResults[epochs] = { 0 };
  size_t actualEpochs = 0;

  for (; actualEpochs < epochs; ++actualEpochs) {
    const auto maxIters = uint32_t(FLAGS_bm_max_iters);
    for (auto n = uint32_t(FLAGS_bm_min_iters); n < maxIters; n *= 2) {
      auto const nsecsAndIter = fun(static_cast<unsigned int>(n));
      if (nsecsAndIter.first < minNanoseconds) {
        continue;
      }
      // We got an accurate enough timing, done. But only save if
      // smaller than the current result.
      epochResults[actualEpochs] =
          max(0.0, double(nsecsAndIter.first) / nsecsAndIter.second
                    - globalBaseline);
      // Done with the current epoch, we got a meaningful timing.
      break;
    }
    uint64_t now = nanoTimestampNow();
    if (now - global >= timeBudget) {
      // No more time budget available.
      ++actualEpochs;
      break;
    }
  }

  // If the benchmark was basically drowned in baseline noise, it's
  // possible it became negative.
  return max(0.0, *min_element(epochResults, epochResults + actualEpochs));
}

struct ScaleInfo {
  double boundary;
  const char* suffix;
};

static const ScaleInfo kTimeSuffixes[] {
  { 365.25 * 24 * 3600, "years" },
  { 24 * 3600, "days" },
  { 3600, "hr" },
  { 60, "min" },
  { 1, "s" },
  { 1E-3, "ms" },
  { 1E-6, "us" },
  { 1E-9, "ns" },
  { 1E-12, "ps" },
  { 1E-15, "fs" },
  { 0, nullptr },
};

static const ScaleInfo kMetricSuffixes[] {
  { 1E24, "Y" },  // yotta
  { 1E21, "Z" },  // zetta
  { 1E18, "X" },  // "exa" written with suffix 'X' so as to not create
                  //   confusion with scientific notation
  { 1E15, "P" },  // peta
  { 1E12, "T" },  // terra
  { 1E9, "G" },   // giga
  { 1E6, "M" },   // mega
  { 1E3, "K" },   // kilo
  { 1, "" },
  { 1E-3, "m" },  // milli
  { 1E-6, "u" },  // micro
  { 1E-9, "n" },  // nano
  { 1E-12, "p" }, // pico
  { 1E-15, "f" }, // femto
  { 1E-18, "a" }, // atto
  { 1E-21, "z" }, // zepto
  { 1E-24, "y" }, // yocto
  { 0, nullptr },
};

static string humanReadable(double n, unsigned int decimals,
                            const ScaleInfo* scales) {
  if (std::isinf(n) || std::isnan(n)) {
    return rdd::to<string>(n);
  }
  const double absValue = fabs(n);
  const ScaleInfo* scale = scales;
  while (absValue < scale[0].boundary && scale[1].suffix != nullptr) {
    ++scale;
  }
  const double scaledValue = n / scale->boundary;
  return stringPrintf("%.*f%s", decimals, scaledValue, scale->suffix);
}

static string readableTime(double n, unsigned int decimals) {
  return humanReadable(n, decimals, kTimeSuffixes);
}

static string readableMetric(double n, unsigned int decimals) {
  return humanReadable(n, decimals, kMetricSuffixes);
}

static void printBenchmarkResults(
  const vector<tuple<string, string, double> >& data) {
  // Width available
  static const unsigned int columns = 76;

  // Print a horizontal rule
  auto separator = [&](char pad) {
    puts(string(columns, pad).c_str());
  };

  // Print header for a file
  auto header = [&](const string& file) {
    separator('=');
    printf("%-*srelative  time/iter  iters/s\n",
           columns - 28, file.c_str());
    separator('=');
  };

  double baselineNsPerIter = numeric_limits<double>::max();
  string lastFile;

  for (auto& datum : data) {
    auto file = get<0>(datum);
    if (file != lastFile) {
      // New file starting
      header(file);
      lastFile = file;
    }

    string s = get<1>(datum);
    if (s == "-") {
      separator('-');
      continue;
    }
    bool useBaseline /* = void */;
    if (s[0] == '%') {
      s.erase(0, 1);
      useBaseline = true;
    } else {
      baselineNsPerIter = get<2>(datum);
      useBaseline = false;
    }
    s.resize(columns - 29, ' ');
    auto nsPerIter = get<2>(datum);
    auto secPerIter = nsPerIter / 1E9;
    auto itersPerSec = (secPerIter == 0)
                           ? std::numeric_limits<double>::infinity()
                           : (1 / secPerIter);
    if (!useBaseline) {
      // Print without baseline
      printf("%*s           %9s  %7s\n",
             static_cast<int>(s.size()), s.c_str(),
             readableTime(secPerIter, 2).c_str(),
             readableMetric(itersPerSec, 2).c_str());
    } else {
      // Print with baseline
      auto rel = baselineNsPerIter / nsPerIter * 100.0;
      printf("%*s %7.2f%%  %9s  %7s\n",
             static_cast<int>(s.size()), s.c_str(),
             rel,
             readableTime(secPerIter, 2).c_str(),
             readableMetric(itersPerSec, 2).c_str());
    }
  }
  separator('=');
}

void runBenchmarks() {
  RDDCHECK(!benchmarks().empty());

  vector<tuple<string, string, double>> results;
  results.reserve(benchmarks().size() - 1);

  // PLEASE KEEP QUIET. MEASUREMENTS IN PROGRESS.

  size_t baselineIndex = getGlobalBenchmarkBaselineIndex();

  auto const globalBaseline =
      runBenchmarkGetNSPerIteration(get<2>(benchmarks()[baselineIndex]), 0);
  for (size_t i = 0; i < benchmarks().size(); i++) {
    if (i == baselineIndex) {
      continue;
    }
    double elapsed = 0.0;
    if (get<1>(benchmarks()[i]) != "-") { // skip separators
      elapsed = runBenchmarkGetNSPerIteration(get<2>(benchmarks()[i]),
                                              globalBaseline);
    }
    results.emplace_back(get<0>(benchmarks()[i]),
                         get<1>(benchmarks()[i]), elapsed);
  }

  // PLEASE MAKE NOISE. MEASUREMENTS DONE.

  printBenchmarkResults(results);
}

} // namespace rdd
