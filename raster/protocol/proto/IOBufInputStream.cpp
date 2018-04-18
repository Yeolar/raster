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

#include "raster/protocol/proto/IOBufInputStream.h"

namespace rdd {

IOBufInputStream::IOBufInputStream(acc::io::Cursor* cursor)
  : cursor_(cursor),
    size_(cursor_->totalLength()),
    position_(0) {
}

bool IOBufInputStream::Next(const void** data, int* size) {
  if (cursor_->isAtEnd()) {
    return false;
  }
  auto rng = cursor_->peekBytes();
  *data = rng.data();
  *size = rng.size();
  position_ += rng.size();
  return true;
}

void IOBufInputStream::BackUp(int count) {
  DCHECK_GE(count, 0);
  position_ -= cursor_->retreatAtMost(count);
}

bool IOBufInputStream::Skip(int count) {
  DCHECK_GE(count, 0);
  int n = cursor_->skipAtMost(count);
  position_ += n;
  return count == n;
}

google::protobuf::int64 IOBufInputStream::ByteCount() const {
  return position_;
}

} // namespace rdd
