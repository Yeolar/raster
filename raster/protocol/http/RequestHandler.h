/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/protocol/http/HTTPException.h"
#include "raster/protocol/http/HTTPRequest.h"
#include "raster/protocol/http/HTTPResponse.h"
#include "raster/util/json.h"

namespace rdd {

class RequestHandler {
public:
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
  virtual void setDefaultHeaders() {}

  void clear();

  void setStatusCode(int code);
  int getStatusCode() const;

  void write(StringPiece sp);
  void write(const dynamic& json);

  void handleException(const HTTPException& e);
  void handleException(const std::exception& e);
  void handleException();

  void sendError(int code = 500);

  std::string getLocale();
  std::string getBrowserLocale();
  virtual std::string getUserLocale() { return ""; }

  HTTPRequest* request;
  HTTPResponse* response;

protected:
  std::string locale_;
};

} // namespace rdd
