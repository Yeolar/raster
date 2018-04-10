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

#include "raster/protocol/http/HTTPException.h"
#include "raster/protocol/http/HTTPMessage.h"
#include "raster/protocol/http/ResponseBuilder.h"

namespace rdd {

class RequestHandler {
 public:
  virtual ~RequestHandler() {}

  virtual void onGet     ();
  virtual void onPost    ();
  virtual void onPut     ();
  virtual void onHead    ();
  virtual void onDelete  ();
  virtual void onConnect ();
  virtual void onOptions ();
  virtual void onTrace   ();

  virtual void prepare() {}
  virtual void finish() {}

  void handleException(const HTTPException& e);
  void handleException(const std::exception& e);
  void handleException();

  void sendError(uint16_t code = 500);

  std::string getLocale();
  std::string getBrowserLocale();
  virtual std::string getUserLocale() { return ""; }

  HTTPMessage* headers;
  acc::IOBuf* body;
  HTTPHeaders* trailers;
  ResponseBuilder response;

 protected:
  std::string locale_;
};

} // namespace rdd
