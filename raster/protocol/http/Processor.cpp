/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/Processor.h"

namespace rdd {

void HTTPMethodProcessor::onGet(HTTPRequest* req, HTTPResponse* resp) {
  throw std::runtime_error("Unimplement HTTP method");
}
void HTTPMethodProcessor::onPost(HTTPRequest* req, HTTPResponse* resp) {
  throw std::runtime_error("Unimplement HTTP method");
}
void HTTPMethodProcessor::onPut(HTTPRequest* req, HTTPResponse* resp) {
  throw std::runtime_error("Unimplement HTTP method");
}
void HTTPMethodProcessor::onHead(HTTPRequest* req, HTTPResponse* resp) {
  throw std::runtime_error("Unimplement HTTP method");
}
void HTTPMethodProcessor::onDelete(HTTPRequest* req, HTTPResponse* resp) {
  throw std::runtime_error("Unimplement HTTP method");
}
void HTTPMethodProcessor::onConnect(HTTPRequest* req, HTTPResponse* resp) {
  throw std::runtime_error("Unimplement HTTP method");
}
void HTTPMethodProcessor::onOptions(HTTPRequest* req, HTTPResponse* resp) {
  throw std::runtime_error("Unimplement HTTP method");
}
void HTTPMethodProcessor::onTrace(HTTPRequest* req, HTTPResponse* resp) {
  throw std::runtime_error("Unimplement HTTP method");
}

} // namespace rdd
