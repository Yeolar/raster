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

#include <memory>

#include "raster/net/Event.h"

namespace rdd {

class Event;

class Processor {
 public:
  Processor(Event* event) : event_(event) {}
  virtual ~Processor() {}

  virtual void run() = 0;

 protected:
  Event* event_;
};

class ProcessorFactory {
 public:
  virtual ~ProcessorFactory() {}
  virtual std::unique_ptr<Processor> create(Event* event) = 0;
};

} // namespace rdd
