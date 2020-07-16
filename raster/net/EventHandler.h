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

#include "raster/event/EventHandlerBase.h"
#include "raster/event/EventLoop.h"

namespace raster {

class EventHandler : public acc::EventHandlerBase {
 public:
  EventHandler(acc::EventLoop* loop) : loop_(loop) {}
  virtual ~EventHandler() {}

  void onConnect(acc::EventBase* event) override;
  void onListen(acc::EventBase* event) override;
  void onRead(acc::EventBase* event) override;
  void onWrite(acc::EventBase* event) override;
  void onTimeout(acc::EventBase* event) override;
  void close(acc::EventBase* event) override;

 private:
  void onComplete(acc::EventBase* event);
  void onError(acc::EventBase* event);

  acc::EventLoop* loop_;
};

} // namespace raster
