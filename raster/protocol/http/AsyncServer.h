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

#include <boost/regex.hpp>

#include "raster/net/Service.h"
#include "raster/protocol/http/Processor.h"

namespace rdd {

class HTTPAsyncServer : public Service {
 public:
  HTTPAsyncServer(StringPiece name) : Service(name) {}
  ~HTTPAsyncServer() override {}

  void makeChannel(int port, const TimeoutOption& timeoutOpt) override;

  template <class T, class ...Args>
  void addHandler(const std::string& regex, Args&&... args) {
    handlers_.emplace(boost::regex(regex),
                      std::make_shared<T>(std::forward<Args>(args)...));
  }

  std::shared_ptr<RequestHandler> matchHandler(const std::string& url) const;

 private:
  std::map<boost::regex, std::shared_ptr<RequestHandler>> handlers_;
};

} // namespace rdd
