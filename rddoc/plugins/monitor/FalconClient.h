/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>

namespace rdd {

class FalconClient {
public:
  static const char* URL;

  explicit FalconClient(const std::string& url = URL);

  ~FalconClient();

  bool send(const std::map<std::string, int>& value);

private:
  bool post(const std::string& data);

  std::string url_;
};

}

