/*
 * Copyright 2020 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/event/Poll.h"

#include "raster/event/EventPoll.h"
#include "raster/event/KqueuePoll.h"
#include "raster/event/SelectPoll.h"

DEFINE_int32(peer_max_count, 10000, "Peers max count.");

namespace raster {

std::unique_ptr<Poll> Poll::create(int size) {
#if __linux__
  return std::unique_ptr<Poll>(new EventPoll(size));
#elif __APPLE__
  return std::unique_ptr<Poll>(new KqueuePoll(size));
#else
  return std::unique_ptr<Poll>(new SelectPoll(size));
#endif
}

Poll::Poll(int size) : size_(size) {
  fired_.resize(size);
  eventMap_.resize(FLAGS_peer_max_count);
}

const Poll::FdMask* Poll::firedFds() const {
  return fired_.data();
}

EventBase*& Poll::event(int i) {
  return eventMap_[i];
}

} // namespace raster
