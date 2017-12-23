/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <google/protobuf/io/zero_copy_stream.h>

#include "raster/io/Cursor.h"

namespace rdd {

class IOBufInputStream : public google::protobuf::io::ZeroCopyInputStream {
 public:
  IOBufInputStream(io::Cursor* cursor);
  ~IOBufInputStream() {}

  // implements ZeroCopyInputStream
  bool Next(const void** data, int* size);
  void BackUp(int count);
  bool Skip(int count);
  google::protobuf::int64 ByteCount() const;

 private:
  io::Cursor* cursor_;
  const int size_;
  int position_;
};

} // namespace rdd
