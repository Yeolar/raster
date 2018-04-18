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

#include "raster/protocol/thrift/Processor.h"

#include "raster/protocol/binary/Transport.h"

namespace rdd {

TProcessor::TProcessor(
    Event* event,
    std::unique_ptr< ::apache::thrift::TProcessor> processor)
  : Processor(event), processor_(std::move(processor)) {
  pibuf_.reset(new apache::thrift::transport::TMemoryBuffer());
  pobuf_.reset(new apache::thrift::transport::TMemoryBuffer());
  piprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pibuf_));
  poprot_.reset(new apache::thrift::protocol::TBinaryProtocol(pobuf_));
}

void TProcessor::run() {
  auto transport = event_->transport<BinaryTransport>();
  try {
    auto range = transport->body->coalesce();
    pibuf_->resetBuffer((uint8_t*)range.data(), range.size());
    processor_->process(piprot_, poprot_, nullptr);
    uint8_t* p;
    uint32_t n;
    pobuf_->getBuffer(&p, &n);
    transport->sendHeader(n);
    transport->sendBody(acc::IOBuf::copyBuffer(p, n));;
  } catch (apache::thrift::protocol::TProtocolException& e) {
    ACCLOG(WARN) << "catch exception: " << e.what();
  } catch (std::exception& e) {
    ACCLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    ACCLOG(WARN) << "catch unknown exception";
  }
}

void TZlibProcessor::run() {
  auto transport = event_->transport<ZlibTransport>();
  try {
    auto range = transport->body->coalesce();
    pibuf_->resetBuffer((uint8_t*)range.data(), range.size());
    processor_->process(piprot_, poprot_, nullptr);
    uint8_t* p;
    uint32_t n;
    pobuf_->getBuffer(&p, &n);
    transport->sendBody(acc::IOBuf::copyBuffer(p, n));;
  } catch (apache::thrift::protocol::TProtocolException& e) {
    ACCLOG(WARN) << "catch exception: " << e.what();
  } catch (std::exception& e) {
    ACCLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    ACCLOG(WARN) << "catch unknown exception";
  }
}

} // namespace rdd
