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

void RequestHandler::clear() {
  response->headers.removeAll();
  response->headers.set(HTTP_HEADER_SERVER, "Raster/1.0");
  response->headers.set(HTTP_HEADER_CONTENT_TYPE, "text/html; charset=UTF-8");
  setDefaultHeaders();
  if (!request->supportHTTP_1_1() &&
      request->headers.getSingleOrEmpty(HTTP_HEADER_CONNECTION) == "Keep-Alive")
    response->headers.set(HTTP_HEADER_CONNECTION, "Keep-Alive");
  response->data->clear();
  response->statusCode = 200;
}

void RequestHandler::setStatusCode(int code) {
  RDDCHECK(isValidResponseCode(code));
  response->statusCode = code;
}

int RequestHandler::getStatusCode() const {
  return response->statusCode;
}

void RequestHandler::write(StringPiece sp) {
  response->appendData(sp);
}

void RequestHandler::write(const dynamic& json) {
  response->headers.set(HTTP_HEADER_CONTENT_TYPE,
                        "application/json; charset=UTF-8");
  response->appendData(toJson(json));
}

void RequestHandler::handleException(const HTTPException& e) {
  RDDLOG(WARN) << e << ", " << *request;
  if (!isValidResponseCode(e.getCode())) {
    RDDLOG(ERROR) << "Bad HTTP status code: " << e.getCode();
    sendError(500);
  } else {
    sendError(e.getCode());
  }
}

void RequestHandler::handleException(const std::exception& e) {
  RDDLOG(ERROR) << "Exception: " << e.what() << ", " << *request;
  sendError(500);
}

void RequestHandler::handleException() {
  RDDLOG(ERROR) << "Unknown exception, " << *request;
  sendError(500);
}

void RequestHandler::sendError(int code) {
  clear();
  setStatusCode(code);
  auto scode = to<std::string>(response->statusCode);
  auto& msg = getResponseW3CName(response->statusCode);
  response->appendData(
      "<html><title>", scode, ": ", msg, "</title>"
      "<body>", scode, ": ", msg, "</body></html>");
}

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

} // namespace rdd
