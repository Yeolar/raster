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
  ~IOBufInputStream() override {}

  // implements ZeroCopyInputStream
  bool Next(const void** data, int* size) override;
  void BackUp(int count) override;
  bool Skip(int count) override;
  google::protobuf::int64 ByteCount() const override;

 private:
  io::Cursor* cursor_;
  const int size_;
  int position_;
};

} // namespace rdd
