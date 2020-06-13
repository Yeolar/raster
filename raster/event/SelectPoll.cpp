/*
 * Copyright 2020 Yeolar
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

#include "raster/event/SelectPoll.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <accelerator/Exception.h>
#include <accelerator/Logging.h>
#include <accelerator/Time.h>

namespace raster {

SelectPoll::SelectPoll(int size) : Poll(size) {
  if (size >= FD_SETSIZE) {
    acc::throwSystemError("exceed FD_SETSIZE");
  }
  FD_ZERO(&rfds_);
  FD_ZERO(&wfds_);
}

SelectPoll::~SelectPoll() {
}

bool SelectPoll::add(int fd, int mask) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  if (mask & kRead) {
    FD_SET(fd, &rfds_);
  }
  if (mask & kWrite) {
    FD_SET(fd, &wfds_);
  }
  eventMap_[fd] = mask;
  return true;
}

bool SelectPoll::modify(int fd, int mask) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  if ((eventMap_[fd] & kRead) != (mask & kRead)) {
    if (mask & kRead) {
      FD_SET(fd, &rfds_);
    } else {
      FD_CLR(fd, &rfds_);
    }
  }
  if ((eventMap_[fd] & kWrite) != (mask & kWrite)) {
    if (mask & kWrite) {
      FD_SET(fd, &wfds_);
    } else {
      FD_CLR(fd, &wfds_);
    }
  }
  eventMap_[fd] = mask;
  return true;
}

bool SelectPoll::remove(int fd) {
  if (fd >= FLAGS_peer_max_count) {
    ACCLOG(ERROR) << "fd exceed: " << fd << ">=" << FLAGS_peer_max_count;
    return false;
  }
  if (eventMap_[fd] & kRead) {
    FD_CLR(fd, &rfds_);
  }
  if (eventMap_[fd] & kWrite) {
    FD_CLR(fd, &wfds_);
  }
  eventMap_[fd] = kNone;
  return true;
}

int SelectPoll::wait(int timeout) {
  fd_set rfds, wfds;
  memcpy(&rfds, &rfds_, sizeof(fd_set));
  memcpy(&wfds, &wfds_, sizeof(fd_set));
  int r;
  if (timeout != -1) {
    struct timeval tv = acc::toTimeval(timeout * 1000);
    r = select(size_, &rfds, &wfds, nullptr, &tv);
  } else {
    r = select(size_, &rfds, &wfds, nullptr, nullptr);
  }
  if (r == -1 && errno != EINTR) {
    ACCPLOG(ERROR) << "select failed";
  }
  int n = 0;
  if (r > 0) {
    for (int i = 0; i < size_; i++) {
      int mask = 0;
      if (eventMap_[i] == kNone) {
        continue;
      }
      if ((eventMap_[i] & kRead) && FD_ISSET(i, &rfds))
        mask |= kRead;
      if ((eventMap_[i] & kWrite) && FD_ISSET(i, &wfds))
        mask |= kWrite;
      fired_[n].fd = i;
      fired_[n].mask = mask;
      n++;
    }
  }
  return n;
}

}  // namespace raster
