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

#include "raster/net/Transport.h"

namespace rdd {

void Transport::getReadBuffer(void** buf, size_t* bufSize) {
  std::pair<void*, uint32_t> readSpace =
    readBuf_.preallocate(kMinReadSize, kMaxReadSize);
  *buf = readSpace.first;
  *bufSize = readSpace.second;
}

void Transport::readDataAvailable(size_t readSize) {
  ACCLOG(V3) << "read completed, bytes=" << readSize;
  readBuf_.postallocate(readSize);
  processReadData();
}

int Transport::readData(Socket* socket) {
  state_ = kOnReading;
  while (state_ != kFinish) {
    void* buf;
    size_t bufSize;
    getReadBuffer(&buf, &bufSize);
    ssize_t r = socket->recv(buf, bufSize);
    if (r <= 0) {
      return r;
    }
    readDataAvailable(r);
    if (state_ == kError) {
      return -1;
    }
  }
  return 1;
}

int Transport::writeData(Socket* socket) {
  if (state_ == kError) {
    return -1;
  }
  const acc::IOBuf* buf;
  while ((buf = writeBuf_.front()) != nullptr && buf->length() != 0) {
    ssize_t r = socket->send(buf->data(), buf->length());
    if (r < 0) {
      return r;
    }
    writeBuf_.trimStart(r);
  }
  return 1;
}

void Transport::clone(Transport* other) {
  state_ = other->state_;
  readBuf_.append(other->readBuf_.front()->clone());
  writeBuf_.append(other->writeBuf_.front()->clone());
}

} // namespace rdd
