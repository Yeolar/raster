/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/protocol/http/HTTPException.h"
#include "raster/protocol/http/HTTPMessage.h"
#include "raster/protocol/http/ResponseBuilder.h"
#include "raster/util/json.h"

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

  void clear();

  void setStatusCode(int code);
  int getStatusCode() const;

  void write(StringPiece sp);
  void writeHtml(StringPiece sp);
  void writeJson(const dynamic& json);
  void writeText(StringPiece sp);

  void handleException(const HTTPException& e);
  void handleException(const std::exception& e);
  void handleException();

  void sendError(uint16_t code = 500);

  std::string getLocale();
  std::string getBrowserLocale();
  virtual std::string getUserLocale() { return ""; }

  HTTPMessage* headers;
  IOBuf* body;
  HTTPHeaders* trailers;
  ResponseBuilder response;

protected:
  std::string locale_;
};

} // namespace rdd
