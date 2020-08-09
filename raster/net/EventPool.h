/*
 * Copyright 2017 Yeolar
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

#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "raster/net/Event.h"
#include "raster/net/NetUtil.h"

namespace raster {

class EventPool {
 public:
  EventPool() {}

  std::unique_ptr<Event> get(const Peer& peer);

  void giveBack(std::unique_ptr<Event> event);

  size_t count() const;

 private:
  std::unordered_map<Peer, std::vector<std::unique_ptr<Event>>> pool_;
  mutable std::mutex lock_;
};

class EventPoolManager {
 public:
  EventPoolManager() {}

  EventPool* getPool(int id);

 private:
  std::unordered_map<int, std::unique_ptr<EventPool>> pool_;
  mutable std::mutex lock_;
};

} // namespace raster
