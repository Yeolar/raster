/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <limits.h>
#include <stdint.h>
#include <chrono>
#include <string>
#include <time.h>
#include <boost/operators.hpp>
#include "raster/util/Conv.h"

namespace rdd {

typedef std::chrono::system_clock Clock;

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
    : cycle_(cycle) {
    start_ = timestampNow();
  }

  void setCycle(uint64_t cycle) {
    cycle_ = cycle;
  }

  bool isExpired() {
    if (cycle_ > 0 && timePassed(start_) > cycle_) {
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
