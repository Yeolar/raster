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

#include "raster/net/Processor.h"
#include "raster/protocol/binary/Transport.h"

namespace rdd {

class BinaryProcessor : public Processor {
 public:
  BinaryProcessor(Event* event) : Processor(event) {}
  ~BinaryProcessor() override {}

  virtual void process(ByteRange& response, const ByteRange& request) = 0;

  void run() override;

 private:
  ByteRange ibuf_;
  ByteRange obuf_;
};

template <class P>
class BinaryProcessorFactory : public ProcessorFactory {
 public:
  BinaryProcessorFactory() {}
  ~BinaryProcessorFactory() override {}

  std::unique_ptr<Processor> create(Event* event) override {
    return make_unique<P>(event);
  }
};

} // namespace rdd
