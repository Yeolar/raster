/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

namespace rdd {

// See http://gcc.gnu.org/ml/gcc/2003-04/msg01180.html for a discussion on
// this idiom. Also see http://en.wikipedia.org/wiki/Memory_ordering.
inline void asm_volatile_memory() {
  asm volatile("" : : : "memory");
}

inline void asm_volatile_pause() {
  asm volatile("pause");
}

} // namespace rdd
