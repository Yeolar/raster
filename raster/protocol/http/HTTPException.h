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

#pragma once

#include <memory>

#include "accelerator/Memory.h"
#include "accelerator/io/IOBuf.h"
#include "raster/net/ErrorEnum.h"
#include "raster/net/Exception.h"
#include "raster/protocol/http/HTTPMessage.h"

namespace raster {

/**
 * This class encapsulates the various errors that can occur on an
 * http session. Errors can occur at various levels: the connection can
 * be closed for reads and/or writes, the message body may fail to parse,
 * or various protocol constraints may be violated.
 */
class HTTPException : public NetException {
 public:
  /**
   * Indicates which direction of the data flow was affected by this
   * exception. For instance, if a class receives HTTPException(INGRESS),
   * then it should consider ingress callbacks finished (whether or not
   * the underlying transport actually shut down). Likewise for
   * HTTPException(EGRESS), the class should consider the write stream
   * shut down. INGRESS_AND_EGRESS indicates both directions are finished.
   */
  enum class Direction {
    INGRESS = 0,
    EGRESS,
    INGRESS_AND_EGRESS,
  };

  explicit HTTPException(Direction dir, const std::string& msg)
      : NetException(msg),
        dir_(dir) {}

  template<typename... Args>
  explicit HTTPException(uint32_t statusCode, Args&&... args)
    : NetException(acc::to<std::string>(std::forward<Args>(args)...)),
      dir_(Direction::INGRESS_AND_EGRESS),
      statusCode_(statusCode) {}

  HTTPException(const HTTPException& ex) :
      NetException(static_cast<const NetException&>(ex)),
      dir_(ex.dir_),
      netError_(ex.netError_),
      statusCode_(ex.statusCode_),
      errno_(ex.errno_) {
    if (ex.currentIngressBuf_) {
      currentIngressBuf_ = std::move(ex.currentIngressBuf_->clone());
    }
    if (ex.partialMsg_) {
      partialMsg_ = std::make_unique<HTTPMessage>(*ex.partialMsg_.get());
    }
  }

  std::string describe() const;

  Direction getDirection() const {
    return dir_;
  }

  bool isIngressException() const {
    return dir_ == Direction::INGRESS ||
      dir_ == Direction::INGRESS_AND_EGRESS;
  }

  bool isEgressException() const {
    return dir_ == Direction::EGRESS ||
      dir_ == Direction::INGRESS_AND_EGRESS;
  }

  bool hasNetError() const {
    return (netError_ != kErrorNone);
  }

  void setNetError(NetError netError) {
    netError_ = netError;
  }

  NetError getNetError() const {
    return netError_;
  }

  bool hasStatusCode() const {
    return (statusCode_ != 0);
  }

  void setStatusCode(uint32_t statusCode) {
    statusCode_ = statusCode;
  }

  uint32_t getStatusCode() const {
    return statusCode_;
  }

  bool hasErrno() const {
    return (errno_ != 0);
  }
  void setErrno(uint32_t errno) {
    errno_ = errno;
  }
  uint32_t getErrno() const {
    return errno_;
  }

  void setCurrentIngressBuf(std::unique_ptr<acc::IOBuf> buf) {
    currentIngressBuf_ = std::move(buf);
  }

  std::unique_ptr<acc::IOBuf> moveCurrentIngressBuf() {
    return std::move(currentIngressBuf_);
  }

  void setPartialMsg(std::unique_ptr<HTTPMessage> partialMsg) {
    partialMsg_ = std::move(partialMsg);
  }

  std::unique_ptr<HTTPMessage> movePartialMsg() {
    return std::move(partialMsg_);
  }

 private:
  Direction dir_;
  NetError netError_{kErrorNone};
  uint32_t statusCode_{0};
  uint32_t errno_{0};
  // current ingress buffer, may be compressed
  std::unique_ptr<acc::IOBuf> currentIngressBuf_;
  // partial message that is being parsed
  std::unique_ptr<HTTPMessage> partialMsg_;
};

std::ostream& operator<<(std::ostream& os, const HTTPException& ex);

} // namespace raster
