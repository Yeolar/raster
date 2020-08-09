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

#include "raster/protocol/binary/Processor.h"

namespace raster {

void BinaryProcessor::run() {
  auto transport = event_->transport<BinaryTransport>();
  try {
    ibuf_ = transport->body->coalesce();
    process(obuf_, ibuf_);
    transport->sendHeader(obuf_.size());
    transport->sendBody(acc::IOBuf::copyBuffer(obuf_));
  } catch (std::exception& e) {
    ACCLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    ACCLOG(WARN) << "catch unknown exception";
  }
}

} // namespace raster
