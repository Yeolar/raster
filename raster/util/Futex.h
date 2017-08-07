/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <limits>
#include <assert.h>
#include <unistd.h>
#include "raster/util/noncopyable.h"
#include "raster/util/Time.h"

namespace rdd {

enum class FutexResult {
  VALUE_CHANGED, /* Futex value didn't match expected */
  AWOKEN,        /* futex wait matched with a futex wake */
  INTERRUPTED,   /* Spurious wake-up or signal caused futex wait failure */
  TIMEDOUT
};

/**
 * Futex is an atomic 32 bit unsigned integer that provides access to the
 * futex() syscall on that value.  It is templated in such a way that it
 * can interact properly with DeterministicSchedule testing.
 *
 * If you don't know how to use futex(), you probably shouldn't be using
 * this class.  Even if you do know how, you should have a good reason
 * (and benchmarks to back you up).
 */
struct Futex : std::atomic<uint32_t>, noncopyable {

  explicit Futex(uint32_t init = 0) : std::atomic<uint32_t>(init) {}

  /** Puts the thread to sleep if this->load() == expected.  Returns true when
   *  it is returning because it has consumed a wake() event, false for any
   *  other return (signal, this->load() != expected, or spurious wakeup). */
  bool futexWait(uint32_t expected, uint32_t waitMask = -1) {
    auto rv = futexWaitImpl(expected, nullptr, waitMask);
    assert(rv != FutexResult::TIMEDOUT);
    return rv == FutexResult::AWOKEN;
  }

  /** Similar to futexWait but also accepts a timeout that gives the time until
   *  when the call can block (time is the absolute time i.e time since epoch).
   *  Returns one of FutexResult values.  */
  FutexResult futexWaitUntil(uint32_t expected,
                             const struct timespec& absTime,
                             uint32_t waitMask = -1) {
    struct timespec timeout = absTime;
    return futexWaitImpl(expected, &timeout, waitMask);
  }

  /** Wakens up to count waiters where (waitMask & wakeMask) !=
   *  0, returning the number of awoken threads, or -1 if an error
   *  occurred.  Note that when constructing a concurrency primitive
   *  that can guard its own destruction, it is likely that you will
   *  want to ignore EINVAL here (as well as making sure that you
   *  never touch the object after performing the memory store that
   *  is the linearization point for unlock or control handoff).
   *  See https://sourceware.org/bugzilla/show_bug.cgi?id=13690 */
  int futexWake(int count = std::numeric_limits<int>::max(),
                uint32_t wakeMask = -1);

 private:

  /** Underlying implementation of futexWait and futexWaitUntil. */
  FutexResult futexWaitImpl(uint32_t expected,
                            struct timespec* timeout,
                            uint32_t waitMask);
};

} // namespace rdd
