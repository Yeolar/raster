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

#include <google/protobuf/io/zero_copy_stream.h>

#include "accelerator/io/Cursor.h"

namespace rdd {

class IOBufInputStream : public google::protobuf::io::ZeroCopyInputStream {
 public:
  IOBufInputStream(acc::io::Cursor* cursor);
  ~IOBufInputStream() override {}

  // implements ZeroCopyInputStream
  bool Next(const void** data, int* size) override;
  void BackUp(int count) override;
  bool Skip(int count) override;
  google::protobuf::int64 ByteCount() const override;

 private:
  acc::io::Cursor* cursor_;
  const int size_;
  int position_;
};

} // namespace rdd
