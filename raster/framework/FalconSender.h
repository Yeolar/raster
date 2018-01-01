/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>

#include "raster/framework/Monitor.h"

namespace rdd {

class FalconSender : public Monitor::Sender {
 public:
  static const char* URL;

  explicit FalconSender(const std::string& url = URL);

  ~FalconSender();

  bool send(const Monitor::MonMap& value) override;

 private:
  bool post(const std::string& data);

  std::string url_;
};

} // namespace rdd
