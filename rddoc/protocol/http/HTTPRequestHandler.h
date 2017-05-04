/*
 * Copyright (C) 2017, Yeolar
 */

namespace rdd {

class HTTPRequestHandler {
public:
  HTTPRequestHandler() {}
  virtual ~HTTPRequestHandler() {}

  virtual void GET(const std::string& uri) {
  }

  virtual void POST(const std::string& uri) {
  }

  virtual void PUT(const std::string& uri) {
  }

  virtual void HEAD(const std::string& uri) {
  }

  virtual void DELETE(const std::string& uri) {
  }

  virtual void TRACE(const std::string& uri) {
  }

private:
};

}
