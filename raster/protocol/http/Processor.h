/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <boost/regex.hpp>

#include "raster/net/Processor.h"
#include "raster/protocol/http/RequestHandler.h"

namespace rdd {

class HTTPProcessor : public Processor {
public:
  HTTPProcessor(
      Event* event,
      const std::shared_ptr<RequestHandler>& handler)
    : Processor(event), handler_(handler) {}

  virtual ~HTTPProcessor() {}

  virtual void run();

protected:
  std::shared_ptr<RequestHandler> handler_;
};

class HTTPProcessorFactory : public ProcessorFactory {
public:
  HTTPProcessorFactory(const std::map<std::string, std::string>& routers);
  virtual ~HTTPProcessorFactory() {}

  virtual std::unique_ptr<Processor> create(Event* event);

private:
  std::map<std::string, boost::regex> routers_;
};

} // namespace rdd
