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

#include "raster/3rd/thrift/transport/TBufferTransports.h"

namespace rdd {
namespace thrift {

void setSeqId(::apache::thrift::transport::TMemoryBuffer* buf, int32_t seqid);

int32_t getSeqId(::apache::thrift::transport::TMemoryBuffer* buf);

} // namespace thrift
} // namespace rdd
