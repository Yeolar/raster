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

#include "raster/protocol/binary/SyncClient.h"

#include <accelerator/Logging.h>

namespace raster {

BinarySyncClient::BinarySyncClient(const ClientOption& option)
  : peer_(option.peer), timeout_(option.timeout) {
  init();
}

BinarySyncClient::BinarySyncClient(const Peer& peer,
                                   const TimeoutOption& timeout)
  : peer_(peer), timeout_(timeout) {
  init();
}

BinarySyncClient::BinarySyncClient(const Peer& peer,
                                   uint64_t ctimeout,
                                   uint64_t rtimeout,
                                   uint64_t wtimeout)
  : peer_(peer), timeout_({ctimeout, rtimeout, wtimeout}) {
  init();
}

BinarySyncClient::~BinarySyncClient() {
  close();
}

void BinarySyncClient::close() {
  if (transport_->isOpen()) {
    transport_->close();
  }
}

bool BinarySyncClient::connect() {
  try {
    transport_->open();
  }
  catch (std::exception& e) {
    ACCLOG(ERROR) << "BinarySyncClient: connect " << peer_
      << " failed, " << e.what();
    return false;
  }
  ACCLOG(DEBUG) << "connect peer[" << peer_ << "]";
  return true;
}

bool BinarySyncClient::connected() const {
  return transport_->isOpen();
}

bool BinarySyncClient::fetch(acc::ByteRange& response, const acc::ByteRange& request) {
  try {
    transport_->send(request);
    transport_->recv(response);
  }
  catch (std::exception& e) {
    ACCLOG(ERROR) << "BinarySyncClient: fetch " << peer_
      << " failed, " << e.what();
    return false;
  }
  return true;
}

void BinarySyncClient::init() {
  transport_.reset(new BinarySyncTransport(peer_, timeout_));
  ACCLOG(DEBUG) << "SyncClient: " << peer_;
}

} // namespace raster
