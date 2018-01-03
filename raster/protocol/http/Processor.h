/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/Processor.h"
#include "raster/protocol/http/RequestHandler.h"

namespace rdd {

class HTTPProcessor : public Processor {
 public:
  HTTPProcessor(
      Event* event,
      const std::shared_ptr<RequestHandler>& handler)
    : Processor(event), handler_(handler) {}

  ~HTTPProcessor() override {}

  void run() override;

 protected:
  std::shared_ptr<RequestHandler> handler_;
};

class HTTPProcessorFactory : public ProcessorFactory {
 public:
  typedef std::function<
    std::shared_ptr<RequestHandler>(const std::string&)> Router;

  HTTPProcessorFactory(Router router) : router_(std::move(router)) {}
  ~HTTPProcessorFactory() override {}

  std::unique_ptr<Processor> create(Event* event) override;

 private:
  Router router_;
};

} // namespace rdd
