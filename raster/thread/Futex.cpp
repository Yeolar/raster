/*
 * Copyright 2017 Facebook, Inc.
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

#include "raster/thread/Futex.h"

#include <cerrno>
#include <linux/futex.h>
#include <sys/syscall.h>

namespace rdd {

namespace {

////////////////////////////////////////////////////
// native implementation using the futex() syscall

int nativeFutexWake(void* addr, int count, uint32_t wakeMask) {
  int rv = syscall(__NR_futex,
                   addr, /* addr1 */
                   FUTEX_WAKE_BITSET | FUTEX_PRIVATE_FLAG, /* op */
                   count, /* val */
                   nullptr, /* timeout */
                   nullptr, /* addr2 */
                   wakeMask); /* val3 */

  /* NOTE: we ignore errors on wake for the case of a futex
     guarding its own destruction, similar to this
     glibc bug with sem_post/sem_wait:
     https://sourceware.org/bugzilla/show_bug.cgi?id=12674 */
  if (rv < 0) {
    return 0;
  }
  return rv;
}

FutexResult nativeFutexWaitImpl(void* addr,
                                uint32_t expected,
                                struct timespec* timeout,
                                uint32_t waitMask) {
  // Unlike FUTEX_WAIT, FUTEX_WAIT_BITSET requires an absolute timeout
  // value - http://locklessinc.com/articles/futex_cheat_sheet/
  int rv = syscall(__NR_futex,
                   addr, /* addr1 */
                   FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG
                   | FUTEX_CLOCK_REALTIME, /* op */
                   expected, /* val */
                   timeout, /* timeout */
                   nullptr, /* addr2 */
                   waitMask); /* val3 */

  if (rv == 0) {
    return FutexResult::AWOKEN;
  } else {
    switch(errno) {
      case ETIMEDOUT:
        assert(timeout != nullptr);
        return FutexResult::TIMEDOUT;
      case EINTR:
        return FutexResult::INTERRUPTED;
      case EWOULDBLOCK:
        return FutexResult::VALUE_CHANGED;
      default:
        assert(false);
        // EINVAL, EACCESS, or EFAULT.  EINVAL means there was an invalid
        // op (should be impossible) or an invalid timeout (should have
        // been sanitized by timeSpecFromTimePoint).  EACCESS or EFAULT
        // means *addr points to invalid memory, which is unlikely because
        // the caller should have segfaulted already.  We can either
        // crash, or return a value that lets the process continue for
        // a bit. We choose the latter. VALUE_CHANGED probably turns the
        // caller into a spin lock.
        return FutexResult::VALUE_CHANGED;
    }
  }
}

} // namespace

int Futex::futexWake(int count, uint32_t wakeMask) {
  return nativeFutexWake(this, count, wakeMask);
}

FutexResult Futex::futexWaitImpl(uint32_t expected,
                                 struct timespec* timeout,
                                 uint32_t waitMask) {
  return nativeFutexWaitImpl(this, expected, timeout, waitMask);
}

} // namespace rdd
