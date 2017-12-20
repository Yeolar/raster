/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/HTTPEvent.h"
#include "raster/protocol/http/HTTPMethod.h"
#include "raster/protocol/http/Processor.h"
#include "raster/util/ReflectObject.h"
#include "raster/util/ScopeGuard.h"

namespace rdd {

bool HTTPProcessor::run() {
  HTTPEvent* httpev = event<HTTPEvent>();
  handler_->message = httpev->message();
  handler_->body = httpev->body();
  handler_->trailers = httpev->trailers();
  handler_->response.setupTransport(httpev);

  SCOPE_EXIT {
    httpev->pushWriteData();
  };

  try {
    handler_->prepare();
    switch (httpev->message()->getMethod()) {
      case HTTPMethod::GET:
        handler_->onGet();
        break;
      case HTTPMethod::POST:
        handler_->onPost();
        break;
      case HTTPMethod::PUT:
        handler_->onPut();
        break;
      case HTTPMethod::HEAD:
        handler_->onHead();
        break;
      case HTTPMethod::DELETE:
        handler_->onDelete();
        break;
      case HTTPMethod::CONNECT:
        handler_->onConnect();
        break;
      case HTTPMethod::OPTIONS:
        handler_->onOptions();
        break;
      case HTTPMethod::TRACE:
        handler_->onTrace();
        break;
    }
    handler_->finish();
    return true;
  } catch (HTTPException& e) {
    handler_->handleException(e);
  } catch (std::exception& e) {
    handler_->handleException(e);
  } catch (...) {
    handler_->handleException();
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
  auto url = httpev->message()->getURL();
  for (auto& kv : routers_) {
    boost::cmatch match;
    if (boost::regex_match(url.c_str(), match, kv.second)) {
      return std::shared_ptr<Processor>(
        new HTTPProcessor(
            event,
            makeSharedReflectObject<RequestHandler>(kv.first)));
    }
  }
  return std::shared_ptr<Processor>(
    new HTTPProcessor(event, std::make_shared<RequestHandler>()));
}

} // namespace rdd
