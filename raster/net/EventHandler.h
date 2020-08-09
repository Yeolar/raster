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

#include "raster/event/EventHandlerBase.h"
#include "raster/event/EventLoop.h"

namespace raster {

class EventHandler : public EventHandlerBase {
 public:
  EventHandler(EventLoop* loop) : loop_(loop) {}
  virtual ~EventHandler() {}

  void onConnect(EventBase* event) override;
  void onListen(EventBase* event) override;
  void onRead(EventBase* event) override;
  void onWrite(EventBase* event) override;
  void onTimeout(EventBase* event) override;
  void close(EventBase* event) override;

 private:
  void onComplete(EventBase* event);
  void onError(EventBase* event);

  EventLoop* loop_;
};

} // namespace raster
