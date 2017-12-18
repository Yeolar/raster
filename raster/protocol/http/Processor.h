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
  virtual void prepare();

  virtual void onGet     ();
  virtual void onPost    ();
  virtual void onPut     ();
  virtual void onHead    ();
  virtual void onDelete  ();
  virtual void onConnect ();
  virtual void onOptions ();
  virtual void onTrace   ();

  HTTPRequest* request;
  HTTPResponse* response;
};

class HTTPProcessor : public Processor {
public:
  HTTPProcessor(
      Event* event,
      const std::shared_ptr<HTTPMethodProcessor>& processor)
    : Processor(event), processor_(processor) {}

  virtual ~HTTPProcessor() {}

  virtual bool decodeData() {
    return true;
  }

  virtual bool encodeData() {
    return true;
  }

  virtual bool run();

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
