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

#include "raster/protocol/binary/SyncTransport.h"

#include "raster/io/IOBuf.h"

namespace rdd {

void BinarySyncTransport::open() {
  socket_ = Socket::createSyncSocket();
  socket_->setConnTimeout(timeout_.ctimeout);
  socket_->setRecvTimeout(timeout_.rtimeout);
  socket_->setSendTimeout(timeout_.wtimeout);
  socket_->connect(peer_);
}

bool BinarySyncTransport::isOpen() {
  return socket_->isConnected();
}

void BinarySyncTransport::close() {
  socket_->close();
}

void BinarySyncTransport::send(const ByteRange& request) {
  sendHeader(request.size());
  sendBody(IOBuf::copyBuffer(request));
  writeData(socket_.get());
}

void BinarySyncTransport::recv(ByteRange& response) {
  readData(socket_.get());
  response = body->coalesce();
}

} // namespace rdd
