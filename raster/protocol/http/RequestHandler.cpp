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

#include "raster/protocol/http/RequestHandler.h"

namespace raster {

void RequestHandler::onGet() {
  throw HTTPException(405);
}
void RequestHandler::onPost() {
  throw HTTPException(405);
}
void RequestHandler::onPut() {
  throw HTTPException(405);
}
void RequestHandler::onHead() {
  throw HTTPException(405);
}
void RequestHandler::onDelete() {
  throw HTTPException(405);
}
void RequestHandler::onConnect() {
  throw HTTPException(405);
}
void RequestHandler::onOptions() {
  throw HTTPException(405);
}
void RequestHandler::onTrace() {
  throw HTTPException(405);
}

void RequestHandler::handleException(const HTTPException& e) {
  ACCLOG(WARN) << e;
  sendError(e.getStatusCode());
}

void RequestHandler::handleException(const std::exception& e) {
  ACCLOG(ERROR) << "Exception: " << e.what();
  sendError(500);
}

void RequestHandler::handleException() {
  ACCLOG(ERROR) << "Unknown exception";
  sendError(500);
}

void RequestHandler::sendError(uint16_t code) {
  response
    .status(code)
    .body(code)
    .body(": ")
    .body(HTTPMessage::getDefaultReason(code))
    .body("\r\n")
    .sendWithEOM();
}

#if 0
std::string RequestHandler::getLocale() {
  if (locale_.empty()) locale_ = getUserLocale();
  if (locale_.empty()) locale_ = getBrowserLocale();
  ACCCHECK(!locale_.empty());
  return locale_;
}

std::string RequestHandler::getBrowserLocale() {
  acc::StringPiece languages(
      request->headers.getSingleOrEmpty(HTTP_HEADER_ACCEPT_LANGUAGE));
  std::vector<std::pair<acc::StringPiece, float>> locales;
  while (!languages.empty()) {
    auto lang = languages.split_step(',');
    std::vector<acc::StringPiece> parts;
    split(';', lang, parts);
    float score;
    if (parts.size() > 1 && parts[1].startsWith("q=")) {
      try {
        score = to<float>(parts[1].subpiece(2));
      } catch (...) {
        score = 0;
      }
    } else {
      score = 1.0;
    }
    locales.emplace_back(parts[0], score);
  }
  if (!locales.empty()) {
    std::sort(locales.begin(), locales.end(),
              [](const std::pair<acc::StringPiece, float>& p,
                 const std::pair<acc::StringPiece, float>& q) {
      return p.second > q.second;
    });
    return locales[0].first.str();
  }
  return "en_US";
}
#endif

} // namespace raster
