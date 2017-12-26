/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/thread/SharedMutex.h"

namespace rdd {
// Explicitly instantiate SharedMutex here:
template class SharedMutexImpl<true>;
template class SharedMutexImpl<false>;
} // namespace rdd
