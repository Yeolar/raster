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

#include "raster/protocol/http/AsyncServer.h"

namespace rdd {

void HTTPAsyncServer::makeChannel(int port, const TimeoutOption& timeoutOpt) {
  Peer peer;
  peer.setFromLocalPort(port);
  channel_ = std::make_shared<Channel>(
      peer,
      timeoutOpt,
      make_unique<HTTPTransportFactory>(TransportDirection::DOWNSTREAM),
      make_unique<HTTPProcessorFactory>(
          [&](const std::string& url) {
            return matchHandler(url);
          }));
}

std::shared_ptr<RequestHandler>
HTTPAsyncServer::matchHandler(const std::string& url) const {
  for (auto& kv : handlers_) {
    boost::cmatch match;
    if (boost::regex_match(url.c_str(), match, kv.first)) {
      return kv.second;
    }
  }
  return std::make_shared<RequestHandler>();
}

} // namespace rdd
