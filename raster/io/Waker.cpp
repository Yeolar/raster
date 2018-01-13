/*
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

#include "raster/io/Waker.h"

#include <fcntl.h>
#include <unistd.h>

#include "raster/io/FileUtil.h"
#include "raster/util/Logging.h"

namespace rdd {

Waker::Waker() {
  if (::pipe2(pipeFds_, O_CLOEXEC | O_NONBLOCK) == -1) {
    RDDPLOG(ERROR) << "pipe2 failed";
  }
}

void Waker::wake() const {
  writeNoInt(pipeFds_[1], (void*)"x", 1);
  RDDLOG(V2) << *this << " wake";
}

void Waker::consume() const {
  char c;
  while (readNoInt(pipeFds_[0], &c, 1) > 0) {}
  RDDLOG(V2) << *this << " consume";
}

void Waker::close() {
  ::close(pipeFds_[0]);
  ::close(pipeFds_[1]);
}

std::ostream& operator<<(std::ostream& os, const Waker& waker) {
  os << "Waker(" << waker.fd() << ":" << waker.fd2() << ")";
  return os;
}

} // namespace rdd
