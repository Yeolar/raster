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

#include "raster/io/FlatDict.h"
#include "FlatDictTest_generated.h"
#include <unordered_map>
#include "raster/util/Benchmark.h"
#include "raster/util/Range.h"
#include "raster/util/RWLock.h"

using namespace rdd;

template <class Key>
class LockedUnorderedMap {
public:
  typedef typename FlatDict<Key>::Block Block;

  explicit LockedUnorderedMap(size_t) {}

  Block get(Key key) {
    RLockGuard guard(lock_);
    std::unique_ptr<IOBuf> buf;
    auto it = map_.find(key);
    if (it != map_.end()) {
      buf = std::move(it->second->cloneOne());
    }
    return Block(std::move(buf));
  }

  void erase(Key key) {
    WLockGuard guard(lock_);
    map_.erase(key);
  }

  void update(Key key, ByteRange range, uint64_t ts = timestampNow()) {
    WLockGuard guard(lock_);
    std::unique_ptr<IOBuf> buf(
        IOBuf::createCombined(Block::kHeadSize + range.size()));
    io::Appender appender(buf.get(), 0);
    appender.write(key);
    appender.write(ts);
    appender.push(range);
    map_[key] = std::move(buf);
  }

private:
  std::unordered_map<Key, std::unique_ptr<IOBuf>> map_;
  mutable RWLock lock_;
};

template <typename Map>
void contendedRW(size_t itersPerThread,
                 size_t capacity,
                 size_t numThreads,
                 size_t readsPerWrite) {
  std::unique_ptr<Map> ptr;
  std::atomic<bool> go;
  std::vector<std::thread> threads;
  ByteRange range;

  BENCHMARK_SUSPEND {
    ptr.reset(new Map(capacity));
    while (threads.size() < numThreads) {
      threads.emplace_back([&]() {
        while (!go) {
          std::this_thread::yield();
        }
        size_t reads = 0;
        size_t writes = 0;
        while (reads + writes < itersPerThread) {
          auto r = Random::rand32();
          uint64_t key = (reads + writes) ^ r;
          if (reads < writes * readsPerWrite ||
              writes >= capacity / numThreads) {
            ++reads;
            auto b = ptr->get(key);
            rdd::doNotOptimizeAway(b);
          } else {
            ++writes;
            try {
              ptr->update(key, range);
            } catch (std::bad_alloc&) {
              RDDLOG(INFO) << "bad alloc";
            }
          }
        }
      });
    }
  }
  go = true;

  for (auto& thr : threads) {
    thr.join();
  }

  BENCHMARK_SUSPEND {
    ptr.reset(nullptr);
  }
}

void contendedRW_lockedmap(
    size_t itersPerThread,
    size_t capacity,
    size_t numThreads,
    size_t readsPerWrite) {
  contendedRW<LockedUnorderedMap<uint64_t>>(
      itersPerThread, capacity, numThreads, readsPerWrite);
}

void contendedRW_flatdict(
    size_t itersPerThread,
    size_t capacity,
    size_t numThreads,
    size_t readsPerWrite) {
  contendedRW<FlatDict<uint64_t>>(
      itersPerThread, capacity, numThreads, readsPerWrite);
}

// sudo nice -n -20 ./raster/io/test/raster_io_FlatDictBenchmark -bm_min_iters 1000000
// ============================================================================
// FlatDictBenchmark.cpp                           relative  time/iter  iters/s
// ============================================================================
// contendedRW_lockedmap(small_32thr_99pct)                     7.97us  125.43K
// contendedRW_lockedmap(large_32thr_99pct)                     6.81us  146.85K
// contendedRW_lockedmap(large_32thr_999pct)                    8.68us  115.17K
// contendedRW_flatdict(small_32thr_99pct)                    513.22ns    1.95M
// contendedRW_flatdict(large_32thr_99pct)                    858.63ns    1.16M
// contendedRW_flatdict(large_32thr_999pct)                   880.58ns    1.14M
// ============================================================================

BENCHMARK_NAMED_PARAM(contendedRW_lockedmap, small_32thr_99pct, 100000, 32, 99)
BENCHMARK_NAMED_PARAM(contendedRW_lockedmap, large_32thr_99pct, 100000000, 32, 99)
BENCHMARK_NAMED_PARAM(contendedRW_lockedmap, large_32thr_999pct, 100000000, 32, 999)

BENCHMARK_NAMED_PARAM(contendedRW_flatdict, small_32thr_99pct, 100000, 32, 99)
BENCHMARK_NAMED_PARAM(contendedRW_flatdict, large_32thr_99pct, 100000000, 32, 99)
BENCHMARK_NAMED_PARAM(contendedRW_flatdict, large_32thr_999pct, 100000000, 32, 999)

BENCHMARK_DRAW_LINE();

// sudo nice -n -20 ./raster/io/test/raster_io_FlatDictBenchmark
// ============================================================================
// FlatDictBenchmark.cpp                           relative  time/iter  iters/s
// ============================================================================
// locked_unordered_map                                         3.24ms   308.61
// flatdict                                                     1.96ms   509.87
// flatdict_64                                                  1.97ms   507.16
// ============================================================================

BENCHMARK(locked_unordered_map) {
  ByteRange range;
  LockedUnorderedMap<long> m(10000);
  for (int i=0; i<10000; ++i) {
    m.update(i,range);
  }
  for (int i=0; i<10000; ++i) {
    auto a = m.get(i);
    rdd::doNotOptimizeAway(a);
  }
}

BENCHMARK(flatdict) {
  ByteRange range;
  FlatDict<long> m(10000);
  for (int i=0; i<10000; ++i) {
    m.update(i,range);
  }
  for (int i=0; i<10000; ++i) {
    auto a = m.get(i);
    rdd::doNotOptimizeAway(a);
  }
}

BENCHMARK(flatdict_64) {
  ByteRange range;
  FlatDict64<long> m(10000);
  for (int i=0; i<10000; ++i) {
    m.update(i,range);
  }
  for (int i=0; i<10000; ++i) {
    auto a = m.get(i);
    rdd::doNotOptimizeAway(a);
  }
}

BENCHMARK_DRAW_LINE();

const char* sample =
  "0123456789abcdefghijklmnopqrstuv"
  "wxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_=";
FlatDict64<long> m(16777216, "flatdict.dump");

// ./raster/io/test/raster_io_FlatDictBenchmark -bm_min_iters 1 -bm_max_iters 2
// ============================================================================
// FlatDictBenchmark.cpp                           relative  time/iter  iters/s
// ============================================================================
// flatdict_sync                                                 9.99s  100.13m
// ============================================================================

BENCHMARK(flatdict_sync) {
  BENCHMARK_SUSPEND {
    for (int i=0; i<16777216; ++i) {
      m.update(i,ByteRange(StringPiece(sample)));
    }
  }
  m.sync();
}

int main(int argc, char ** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  rdd::runBenchmarks();
  return 0;
}
