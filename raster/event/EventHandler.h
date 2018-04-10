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

#pragma once

#include "raster/event/Event.h"

namespace rdd {

class EventLoop;

class EventHandler {
 public:
  EventHandler(EventLoop* loop) : loop_(loop) {}

  void onListen(Event* event);
  void onConnect(Event* event);
  void onRead(Event* event);
  void onWrite(Event* event);
  void onComplete(Event* event);
  void onTimeout(Event* event);
  void onError(Event* event);

  void closePeer(Event* event);

 private:
  EventLoop* loop_;
};

} // namespace rdd
