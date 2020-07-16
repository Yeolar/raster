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

#include "raster/net/EventPool.h"

namespace raster {

std::unique_ptr<Event> EventPool::get(const Peer& peer) {
  std::lock_guard<std::mutex> guard(lock_);
  auto& q = pool_[peer];
  if (!q.empty()) {
    auto event = std::move(q.back());
    q.pop_back();
    return event;
  }
  return nullptr;
}

void EventPool::giveBack(std::unique_ptr<Event> event) {
  std::lock_guard<std::mutex> guard(lock_);
  pool_[event->peer()].push_back(std::move(event));
}

size_t EventPool::count() const {
  std::lock_guard<std::mutex> guard(lock_);
  size_t n = 0;
  for (auto& kv : pool_) {
    n += kv.second.size();
  }
  return n;
}

EventPool* EventPoolManager::getPool(int id) {
  std::lock_guard<std::mutex> guard(lock_);
  if (pool_.find(id) == pool_.end()) {
    pool_.emplace(id, acc::make_unique<EventPool>());
  }
  return pool_[id].get();
}

} // namespace raster
