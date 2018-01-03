/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/3rd/thrift/transport/TBufferTransports.h"

namespace rdd {
namespace thrift {

void setSeqId(::apache::thrift::transport::TMemoryBuffer* buf, int32_t seqid);

int32_t getSeqId(::apache::thrift::transport::TMemoryBuffer* buf);

} // namespace thrift
} // namespace rdd
