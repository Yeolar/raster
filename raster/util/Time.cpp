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

#include "raster/util/Time.h"

#include <cmath>
#include <cstring>
#include <stdexcept>

#include "raster/util/Logging.h"

namespace rdd {

std::string timePrintf(time_t t, const char *format) {
  std::string output;
  struct tm tm;

  ::localtime_r(&t, &tm);

  size_t formatLen = strlen(format);
  size_t remaining = std::max(32UL, formatLen * 2);
  output.resize(remaining);
  size_t bytesUsed = 0;

  do {
    bytesUsed = strftime(&output[0], remaining, format, &tm);
    if (bytesUsed == 0) {
      remaining *= 2;
      if (remaining > formatLen * 16) {
        throw std::invalid_argument("Maybe a non-output format given");
      }
      output.resize(remaining);
    } else {  // > 0, there was enough room
      break;
    }
  } while (bytesUsed == 0);

  output.resize(bytesUsed);

  return output;
}

bool isSameDay(time_t t1, time_t t2) {
  struct tm tm;

  ::localtime_r(&t1, &tm);
  int y1 = tm.tm_year;
  int d1 = tm.tm_yday;

  ::localtime_r(&t2, &tm);
  int y2 = tm.tm_year;
  int d2 = tm.tm_yday;

  return (y1 == y2 && d1 == d2);
}

} // namespace rdd
