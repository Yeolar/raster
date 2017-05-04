/*
 * XCount.h -- Sparse integer counts stored in 2 bytes.
 */

#pragma once

#include <iostream>
#include <sys/param.h>

namespace srilm {

#ifdef USE_LONGLONG_COUNTS
typedef unsigned int XCountIndex;        // 4 bytes
typedef unsigned long long XCountValue;  // 8 bytes
#else
typedef unsigned short XCountIndex;  // 2 bytes
typedef unsigned long XCountValue;   // 4 bytes
#endif

#ifndef NBBY
#define NBBY 8
#endif

const unsigned XCount_Maxbits = sizeof(XCountIndex) * NBBY - 1;
const XCountIndex XCount_Maxinline = ((XCountIndex)1 << XCount_Maxbits) - 1;
const unsigned XCount_TableSize = ((XCountIndex)1 << 15) - 1;

class XCount {
public:
  XCount(XCountValue value = 0);
  XCount(const XCount &other);
  ~XCount();

  XCount &operator=(const XCount &other);
  XCount &operator+=(XCountValue value) {
    *this = (XCountValue) * this + value;
    return *this;
  };
  XCount &operator+=(XCount &value) {
    *this = (XCountValue) * this + (XCountValue)value;
    return *this;
  };
  XCount &operator-=(XCountValue value) {
    *this = (XCountValue) * this - value;
    return *this;
  };
  XCount &operator-=(XCount &value) {
    *this = (XCountValue) * this - (XCountValue)value;
    return *this;
  };

  operator XCountValue() const;

  void write(std::ostream &str) const;

  static void freeThread();

private:
  XCountIndex count : XCount_Maxbits;
  bool indirect : 1;

  static void freeXCountTableIndex(XCountIndex);
  static XCountIndex getXCountTableIndex();
};

std::ostream &operator<<(std::ostream &str, const XCount &count);

}

