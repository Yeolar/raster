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

#pragma once

#include <chrono>
#include <climits>
#include <cstdint>
#include <ctime>
#include <string>
#include <boost/operators.hpp>

#include "raster/util/Conv.h"

namespace rdd {

typedef std::chrono::high_resolution_clock Clock;

inline uint64_t nanoTimestampNow() {
  return Clock::now().time_since_epoch().count();
}

inline uint64_t nanoTimePassed(uint64_t nts) {
  return nanoTimestampNow() - nts;
}

inline uint64_t timestampNow() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
      Clock::now().time_since_epoch()).count();
}

inline uint64_t timePassed(uint64_t ts) {
  return timestampNow() - ts;
}

std::string timePrintf(time_t t, const char *format);

inline std::string timeNowPrintf(const char *format) {
  time_t t = time(nullptr);
  return timePrintf(t, format);
}

inline struct timeval toTimeval(uint64_t t) {
  return {
    (__time_t)(t / 1000000),
    (__suseconds_t)(t % 1000000)
  };
}

struct Timestamp {
  int stage{INT_MIN};
  uint64_t stamp{0};

  explicit Timestamp(int stg, uint64_t stm = timestampNow())
    : stage(stg), stamp(stm) {}
};

template <class Tgt>
typename std::enable_if<IsSomeString<Tgt>::value>::type
toAppend(const Timestamp& value, Tgt* result) {
  result->append(to<std::string>(value.stage, ':', value.stamp));
}

inline std::ostream& operator<<(std::ostream& os, const Timestamp& ts) {
  os << ts.stage << ':' << ts.stamp;
  return os;
}

template <class T>
struct Timeout : private boost::totally_ordered<Timeout<T>> {
  T* data{nullptr};
  uint64_t deadline{0};
  bool repeat{false};

  Timeout() {}
  Timeout(T* data_, uint64_t deadline_, bool repeat_ = false)
    : data(data_), deadline(deadline_), repeat(repeat_) {}
};

template <class T>
inline bool operator==(const Timeout<T>& lhs, const Timeout<T>& rhs) {
  return lhs.deadline == rhs.deadline;
}

template <class T>
inline bool operator<(const Timeout<T>& lhs, const Timeout<T>& rhs) {
  return lhs.deadline < rhs.deadline;
}

bool isSameDay(time_t t1, time_t t2);

class CycleTimer {
 public:
  explicit CycleTimer(uint64_t cycle = 0)
    : start_(timestampNow()),
      cycle_(cycle) {}

  void setCycle(uint64_t cycle) {
    cycle_ = cycle;
  }

  bool isExpired() {
    if (cycle_ == 0 || timePassed(start_) > cycle_) {
      start_ = timestampNow();
      return true;
    }
    return false;
  }

 private:
  uint64_t start_;
  uint64_t cycle_;
};

} // namespace rdd
