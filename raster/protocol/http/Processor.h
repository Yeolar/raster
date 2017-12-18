/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <boost/regex.hpp>

#include "raster/net/Processor.h"
#include "raster/protocol/http/HTTPEvent.h"
#include "raster/protocol/http/HTTPException.h"
#include "raster/protocol/http/HTTPMethod.h"
#include "raster/util/ReflectObject.h"

namespace rdd {

class HTTPMethodProcessor {
public:
  virtual void onGet     (HTTPRequest* request, HTTPResponse* response);
  virtual void onPost    (HTTPRequest* request, HTTPResponse* response);
  virtual void onPut     (HTTPRequest* request, HTTPResponse* response);
  virtual void onHead    (HTTPRequest* request, HTTPResponse* response);
  virtual void onDelete  (HTTPRequest* request, HTTPResponse* response);
  virtual void onConnect (HTTPRequest* request, HTTPResponse* response);
  virtual void onOptions (HTTPRequest* request, HTTPResponse* response);
  virtual void onTrace   (HTTPRequest* request, HTTPResponse* response);
};

class HTTPProcessor : public Processor {
public:
  HTTPProcessor(
      const std::shared_ptr<HTTPMethodProcessor>& processor)
    : processor_(processor) {
  }

  virtual ~HTTPProcessor() {}

  virtual bool decodeData(Event* event) {
    HTTPEvent* httpev = reinterpret_cast<HTTPEvent*>(event);
    request_ = httpev->request();
    response_ = httpev->response();
    return true;
  }

  virtual bool encodeData(Event* event) {
    return true;
  }

  virtual bool run() {
    try {
      switch (request_->method) {
        case HTTPMethod::GET:
          processor_->onGet(request_, response_);
          break;
        case HTTPMethod::POST:
          processor_->onPost(request_, response_);
          break;
        case HTTPMethod::PUT:
          processor_->onPut(request_, response_);
          break;
        case HTTPMethod::HEAD:
          processor_->onHead(request_, response_);
          break;
        case HTTPMethod::DELETE:
          processor_->onDelete(request_, response_);
          break;
        case HTTPMethod::CONNECT:
          processor_->onConnect(request_, response_);
          break;
        case HTTPMethod::OPTIONS:
          processor_->onOptions(request_, response_);
          break;
        case HTTPMethod::TRACE:
          processor_->onTrace(request_, response_);
          break;
      }
      return true;
    } catch (HTTPException& e) {
      // write
      RDDLOG(WARN) << "catch http exception: " << e;
    } catch (std::exception& e) {
      RDDLOG(WARN) << "catch exception: " << e.what();
    } catch (...) {
      RDDLOG(WARN) << "catch unknown exception";
    }
    return false;
  }

protected:
  std::shared_ptr<HTTPMethodProcessor> processor_;
  HTTPRequest* request_;
  HTTPResponse* response_;
};

class HTTPProcessorFactory : public ProcessorFactory {
public:
  HTTPProcessorFactory(const std::map<std::string, std::string>& routers);
  virtual ~HTTPProcessorFactory() {}

  virtual std::shared_ptr<Processor> create(Event* event);

private:
  std::map<std::string, boost::regex> routers_;
};

} // namespace rdd
