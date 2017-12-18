/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/Processor.h"

namespace rdd {

void HTTPMethodProcessor::onGet() {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onPost() {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onPut() {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onHead() {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onDelete() {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onConnect() {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onOptions() {
  throw HTTPException(405);
}
void HTTPMethodProcessor::onTrace() {
  throw HTTPException(405);
}

bool HTTPProcessor::run() {
  processor_->request = request_ = event<HTTPEvent>()->request();
  processor_->response = response_ = event<HTTPEvent>()->response();
  try {
    switch (request_->method) {
      case HTTPMethod::GET:
        processor_->onGet();
        break;
      case HTTPMethod::POST:
        processor_->onPost();
        break;
      case HTTPMethod::PUT:
        processor_->onPut();
        break;
      case HTTPMethod::HEAD:
        processor_->onHead();
        break;
      case HTTPMethod::DELETE:
        processor_->onDelete();
        break;
      case HTTPMethod::CONNECT:
        processor_->onConnect();
        break;
      case HTTPMethod::OPTIONS:
        processor_->onOptions();
        break;
      case HTTPMethod::TRACE:
        processor_->onTrace();
        break;
    }
    return true;
  } catch (HTTPException& e) {
    response_->write(e);
    // write
    RDDLOG(WARN) << "catch http exception: " << e;
    return true;
  } catch (std::exception& e) {
    RDDLOG(WARN) << "catch exception: " << e.what();
  } catch (...) {
    RDDLOG(WARN) << "catch unknown exception";
  }
  return false;
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
            event,
            makeSharedReflectObject<HTTPMethodProcessor>(kv.first)));
    }
  }
  throw HTTPException(404);
}

} // namespace rdd
