/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/HTTPMethod.h"
#include "raster/protocol/http/Processor.h"
#include "raster/util/ReflectObject.h"
#include "raster/util/ScopeGuard.h"

namespace rdd {

void HTTPProcessor::run() {
  auto transport = event_->transport<HTTPTransport>();
  transport->headers->dumpMessage(logging::LOG_V1);

  handler_->headers = transport->headers.get();
  handler_->body = transport->body.get();
  handler_->trailers = transport->trailers.get();
  handler_->response.setupTransport(transport);

  try {
    handler_->prepare();
    switch (transport->headers->getMethod()) {
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
  } catch (HTTPException& e) {
    handler_->handleException(e);
  } catch (std::exception& e) {
    handler_->handleException(e);
  } catch (...) {
    handler_->handleException();
  }
}

HTTPProcessorFactory::HTTPProcessorFactory(
    const std::map<std::string, std::string>& routers) {
  for (auto& kv : routers) {
    routers_.emplace(kv.first, boost::regex(kv.second));
  }
}

std::unique_ptr<Processor> HTTPProcessorFactory::create(Event* event) {
  auto transport = event->transport<HTTPTransport>();
  auto url = transport->headers->getURL();
  for (auto& kv : routers_) {
    boost::cmatch match;
    if (boost::regex_match(url.c_str(), match, kv.second)) {
      return make_unique<HTTPProcessor>(
          event,
          makeSharedReflectObject<RequestHandler>(kv.first));
    }
  }
  return make_unique<HTTPProcessor>(event, std::make_shared<RequestHandler>());
}

} // namespace rdd
