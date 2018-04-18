/*
 * Copyright 2018 Yeolar
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

#include "raster/protocol/thrift/Util.h"

#include "accelerator/Logging.h"

namespace rdd {
namespace thrift {

void setSeqId(::apache::thrift::transport::TMemoryBuffer* buf, int32_t seqid) {
  uint8_t* b;
  uint32_t n;
  buf->getBuffer(&b, &n);
  int32_t* p = (int32_t*)b;
  int32_t i = ntohl(*p++);
  i = i < 0 ? ntohl(*p++) : i + 1;
  if (n >= i + sizeof(int32_t)) {
    *(int32_t*)((uint8_t*)p + i) = htonl(seqid);
  } else {
    ACCLOG(WARN) << "invalid buf to set seqid";
  }
}

int32_t getSeqId(::apache::thrift::transport::TMemoryBuffer* buf) {
  uint8_t* b;
  uint32_t n;
  buf->getBuffer(&b, &n);
  int32_t* p = (int32_t*)b;
  int32_t i = ntohl(*p++);
  i = i < 0 ? ntohl(*p++) : i + 1;
  if (n >= i + sizeof(int32_t)) {
    return ntohl(*(int32_t*)((uint8_t*)p + i));
  } else {
    ACCLOG(WARN) << "invalid buf to get seqid";
    return 0;
  }
}

} // namespace thrift
} // namespace rdd
