#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "XCount.h"
#include "TLSWrapper.h"

namespace srilm {

static TLSW_ARRAY(XCountValue, xcountTableTLS, XCount_TableSize);
static TLSW_ARRAY(unsigned, refCountsTLS, XCount_TableSize);
static TLSW(XCountIndex, freeListTLS);
static TLSW(bool, initializedTLS);

void XCount::freeThread() {
  TLSW_FREE(xcountTableTLS);
  TLSW_FREE(refCountsTLS);
  TLSW_FREE(freeListTLS);
  TLSW_FREE(initializedTLS);
}

XCountIndex XCount::getXCountTableIndex() {
  bool& initialized = TLSW_GET(initializedTLS);
  XCountIndex& freeList = TLSW_GET(freeListTLS);
  XCountValue* xcountTable = TLSW_GET_ARRAY(xcountTableTLS);
  unsigned* refCounts = TLSW_GET_ARRAY(refCountsTLS);

  if (!initialized) {
    // populate xcountTable free list
    for (XCountIndex i = 0; i < XCount_TableSize; i++) {
      xcountTable[i] = freeList;
      freeList = i;
    }

    initialized = true;
  }

  assert(freeList != XCount_Maxinline);

  XCountIndex result = freeList;
  freeList = xcountTable[freeList];

  refCounts[result] = 1;
  return result;
}

void XCount::freeXCountTableIndex(XCountIndex idx) {
  XCountIndex& freeList = TLSW_GET(freeListTLS);
  XCountValue* xcountTable = TLSW_GET_ARRAY(xcountTableTLS);
  unsigned* refCounts = TLSW_GET_ARRAY(refCountsTLS);

  refCounts[idx]--;
  if (refCounts[idx] == 0) {
    xcountTable[idx] = freeList;
    freeList = idx;
  }
}

XCount::XCount(XCountValue value) : indirect(false) {
  XCountValue* xcountTable = TLSW_GET_ARRAY(xcountTableTLS);

  if (value <= XCount_Maxinline) {
    indirect = false;
    count = value;
  } else {
    indirect = true;
    count = getXCountTableIndex();

    xcountTable[count] = value;
  }
}

XCount::XCount(const XCount& other) {
  unsigned* refCounts = TLSW_GET_ARRAY(refCountsTLS);
  indirect = other.indirect;
  count = other.count;
  if (indirect) {
    refCounts[count]++;
  }
}

XCount::~XCount() {
  if (indirect) {
    freeXCountTableIndex(count);
  }
}

XCount::operator XCountValue() const {
  XCountValue* xcountTable = TLSW_GET_ARRAY(xcountTableTLS);
  if (!indirect) {
    return count;
  } else {
    return xcountTable[count];
  }
}

XCount& XCount::operator=(const XCount& other) {
  unsigned* refCounts = TLSW_GET_ARRAY(refCountsTLS);
  if (&other != this) {
    if (indirect) {
      freeXCountTableIndex(count);
    }

    count = other.count;
    indirect = other.indirect;

    if (other.indirect) {
      refCounts[other.count]++;
    }
  }
  return *this;
}

void XCount::write(std::ostream& str) const {
  str << (XCountValue) * this;
}

std::ostream& operator<<(std::ostream& str, const XCount& count) {
  count.write(str);
  return str;
}

}

