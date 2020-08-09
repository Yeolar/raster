/*
 * Copyright 2018 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>
#include <iostream>
#include <boost/operators.hpp>

namespace raster {

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

struct TimeoutOption {
  uint64_t ctimeout{uint64_t(-1)};    // connect timeout
  uint64_t rtimeout{uint64_t(-1)};    // read timeout
  uint64_t wtimeout{uint64_t(-1)};    // write timeout

  TimeoutOption() {}
  TimeoutOption(uint64_t cto, uint64_t rto, uint64_t wto)
      : ctimeout(cto), rtimeout(rto), wtimeout(wto) {}
};

std::ostream& operator<<(std::ostream& os, const TimeoutOption& timeout);

} // namespace raster
