/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/Time.h"
#include <cmath>
#include <cstring>
#include <ctime>
#include <stdexcept>

namespace rdd {

std::string timePrintf(time_t t, const char *format) {
  std::string output;

  const struct tm *tm = std::localtime(&t);

  size_t formatLen = strlen(format);
  size_t remaining = std::max(32UL, formatLen * 2);
  output.resize(remaining);
  size_t bytesUsed = 0;

  do {
    bytesUsed = strftime(&output[0], remaining, format, tm);
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
  struct tm* tm;
  tm = localtime(&t1);
  int y1 = tm->tm_year;
  int d1 = tm->tm_yday;
  tm = localtime(&t2);
  int y2 = tm->tm_year;
  int d2 = tm->tm_yday;
  return (y1 == y2 && d1 == d2);
}

} // namespace rdd
