/*
 * Copyright 2018 Yeolar
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

#include "raster/event/EventLoop.h"
#include "raster/coroutine/FiberHub.h"
#include "raster/net/Event.h"
#include "raster/net/Group.h"
#include "raster/net/NetUtil.h"

namespace raster {

struct ForwardTarget {
  int port{0};
  Peer fpeer;
  int flow{0};
};

class NetHub : public FiberHub {
 public:
  virtual EventLoop* getEventLoop() = 0;

  void execute(Event* event);

  void addEvent(Event* event);
  void forwardEvent(Event* event, const Peer& peer);

  bool waitGroup(const std::vector<Event*>& events);

  void setForwarding(bool forward);
  void addForwardTarget(ForwardTarget&& t);

 private:
  Group group_;
  bool forwarding_;
  std::vector<ForwardTarget> forwards_;
};

} // namespace raster
