/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <flatbuffers/flatbuffers.h>
#include "raster/util/Range.h"

namespace rdd {
namespace fbs {

template <class T>
bool verifyFlatbuffer(T* object, const ByteRange& range) {
  flatbuffers::Verifier verifier(range.data(), range.size());
  return object->Verify(verifier);
}

} // namespace fbs
} // namespace rdd
