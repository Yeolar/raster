/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <libmemcached/memcached.h>

namespace rdd {

class MemcachedClient {
public:
  MemcachedClient(const std::string& host, int port);

  ~MemcachedClient() {
    close();
  }

  bool setBehavior(memcached_behavior_t flag, uint64_t data);

  bool get(const std::string& key, std::string& value);

  bool set(const std::string& key, const std::string& value,
           time_t expiration = 0);

private:
  void close();

  memcached_st* memc_;
  std::string key_;
};

}

