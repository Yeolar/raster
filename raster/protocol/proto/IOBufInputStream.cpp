/*
* Copyright (C) 2017, Yeolar
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
