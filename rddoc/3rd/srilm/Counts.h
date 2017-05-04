/*
 * Counts.h -- Utility functions for counts
 */

#pragma once

#include <iostream>
#include <stdio.h>

#include "XCount.h"
#include "File.h"

namespace srilm {

#ifdef USE_LONGLONG_COUNTS
typedef unsigned long long Count; /* a count of something */
#else
typedef unsigned long Count; /* a count of something */
#endif
typedef double FloatCount; /* a fractional count */

extern const unsigned FloatCount_Precision;

/*
 * Binary count I/O
 *  Functions return 0 on failure,  number of bytes read/written otherwise
 */

unsigned writeBinaryCount(File &file, unsigned long long count,
                          unsigned minBytes = 0);
unsigned writeBinaryCount(File &file, float count);
unsigned writeBinaryCount(File &file, double count);

inline unsigned writeBinaryCount(File &file, unsigned long count) {
  return writeBinaryCount(file, (unsigned long long)count);
}

inline unsigned writeBinaryCount(File &file, unsigned count) {
  return writeBinaryCount(file, (unsigned long long)count);
}

inline unsigned writeBinaryCount(File &file, unsigned short count) {
  return writeBinaryCount(file, (unsigned long long)count);
}

inline unsigned writeBinaryCount(File &file, XCount count) {
  return writeBinaryCount(file, (unsigned long long)count);
}

unsigned readBinaryCount(File &file, unsigned long long &count);
unsigned readBinaryCount(File &file, float &count);
unsigned readBinaryCount(File &file, double &count);

inline unsigned readBinaryCount(File &file, unsigned long &count) {
  unsigned long long lcount;
  unsigned result = readBinaryCount(file, lcount);
  if (result > 0) {
    count = (unsigned long)lcount;
  }
  return result;
}

inline unsigned readBinaryCount(File &file, unsigned &count) {
  unsigned long long lcount;
  unsigned result = readBinaryCount(file, lcount);
  if (result > 0) {
    count = (unsigned int)lcount;
  }
  return result;
}

inline unsigned readBinaryCount(File &file, unsigned short &count) {
  unsigned long long lcount;
  unsigned result = readBinaryCount(file, lcount);
  if (result > 0) {
    count = (unsigned short)lcount;
  }
  return result;
}

inline unsigned readBinaryCount(File &file, XCount &count) {
  unsigned long long lcount;
  unsigned result = readBinaryCount(file, lcount);
  if (result > 0) {
    count = (XCountValue)lcount;
  }
  return result;
}

}

