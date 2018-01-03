/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/Processor.h"

#include "raster/protocol/http/HTTPMethod.h"

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

std::unique_ptr<Processor> HTTPProcessorFactory::create(Event* event) {
  auto transport = event->transport<HTTPTransport>();
  auto url = transport->headers->getURL();
  return make_unique<HTTPProcessor>(event, router_(url));
}

} // namespace rdd
