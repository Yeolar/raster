/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once
#define RDD_GEN_COMBINE_H

#include "raster/gen/Base.h"

namespace rdd {
namespace gen {
namespace detail {

template<class Container>
class Interleave;

template<class Container>
class Zip;

}  // namespace detail

template<class Source2,
         class Source2Decayed = typename std::decay<Source2>::type,
         class Interleave = detail::Interleave<Source2Decayed>>
Interleave interleave(Source2&& source2) {
  return Interleave(std::forward<Source2>(source2));
}

}  // namespace gen
}  // namespace rdd

#include "raster/gen/Combine-inl.h"
