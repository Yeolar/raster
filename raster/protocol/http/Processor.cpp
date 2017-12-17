/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/Processor.h"

namespace rdd {

void HTTPMethodProcessor::onGet(HTTPRequest* req, HTTPResponse* resp) {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onPost(HTTPRequest* req, HTTPResponse* resp) {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onPut(HTTPRequest* req, HTTPResponse* resp) {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onHead(HTTPRequest* req, HTTPResponse* resp) {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onDelete(HTTPRequest* req, HTTPResponse* resp) {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onConnect(HTTPRequest* req, HTTPResponse* resp) {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onOptions(HTTPRequest* req, HTTPResponse* resp) {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onTrace(HTTPRequest* req, HTTPResponse* resp) {
  throw HTTPException(405);
}

HTTPProcessorFactory::HTTPProcessorFactory(
    const std::map<std::string, std::string>& routers) {
  for (auto& kv : routers) {
    routers_.emplace(kv.first, boost::regex(kv.second));
  }
}

std::shared_ptr<Processor> HTTPProcessorFactory::create(Event* event) {
  HTTPEvent* httpev = reinterpret_cast<HTTPEvent*>(event);
  auto uri = httpev->request()->uri;
  for (auto& kv : routers_) {
    boost::cmatch match;
    if (boost::regex_match(uri.begin(), uri.end(), match, kv.second)) {
      return std::shared_ptr<Processor>(
        new HTTPProcessor(
            makeSharedReflectObject<HTTPMethodProcessor>(kv.first)));
    }
  }
  throw HTTPException(404);
}

} // namespace rdd
