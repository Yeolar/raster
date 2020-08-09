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

#include "raster/net/Processor.h"
#include "raster/protocol/http/RequestHandler.h"

namespace raster {

class HTTPProcessor : public Processor {
 public:
  HTTPProcessor(
      Event* event,
      const std::shared_ptr<RequestHandler>& handler)
    : Processor(event), handler_(handler) {}

  ~HTTPProcessor() override {}

  void run() override;

 protected:
  std::shared_ptr<RequestHandler> handler_;
};

class HTTPProcessorFactory : public ProcessorFactory {
 public:
  typedef std::function<
    std::shared_ptr<RequestHandler>(const std::string&)> Router;

  HTTPProcessorFactory(Router router) : router_(std::move(router)) {}
  ~HTTPProcessorFactory() override {}

  std::unique_ptr<Processor> create(Event* event) override;

 private:
  Router router_;
};

} // namespace raster
