/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/RequestHandler.h"

namespace rdd {

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
  RDDLOG(WARN) << e;
  sendError(e.getStatusCode());
}

void RequestHandler::handleException(const std::exception& e) {
  RDDLOG(ERROR) << "Exception: " << e.what();
  sendError(500);
}

void RequestHandler::handleException() {
  RDDLOG(ERROR) << "Unknown exception";
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
  RDDCHECK(!locale_.empty());
  return locale_;
}

std::string RequestHandler::getBrowserLocale() {
  StringPiece languages(
      request->headers.getSingleOrEmpty(HTTP_HEADER_ACCEPT_LANGUAGE));
  std::vector<std::pair<StringPiece, float>> locales;
  while (!languages.empty()) {
    auto lang = languages.split_step(',');
    std::vector<StringPiece> parts;
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
              [](const std::pair<StringPiece, float>& p,
                 const std::pair<StringPiece, float>& q) {
      return p.second > q.second;
    });
    return locales[0].first.str();
  }
  return "en_US";
}
#endif

} // namespace rdd
