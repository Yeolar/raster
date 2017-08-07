/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/plugins/memcached/MemcachedClient.h"

namespace rdd {

MemcachedClient::MemcachedClient(const std::string& host, int port) {
  memc_ = memcached(nullptr, 0);
  if (memc_) {
    setBehavior(MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);
    setBehavior(MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 3);
    memcached_server_add(memc_, host.c_str(), port);
  }
}

bool MemcachedClient::setBehavior(memcached_behavior_t flag, uint64_t data) {
  return (memcached_success(memcached_behavior_set(memc_, flag, data)));
}

bool MemcachedClient::get(const std::string& key, std::string& value) {
  uint32_t flags = 0;
  memcached_return_t rc;
  size_t n = 0;
  char* buf = memcached_get(memc_, key.c_str(), key.length(), &n, &flags, &rc);
  if (buf != nullptr) {
    value.assign(buf, n);
    free(buf);
    return true;
  }
  return false;
}

bool MemcachedClient::set(const std::string& key, const std::string& value,
                          time_t expiration) {
  return memcached_success(
      memcached_set(
          memc_,
          key.c_str(), key.length(),
          value.c_str(), value.length(),
          expiration, 0));
}

void MemcachedClient::close() {
  if (memc_) {
    memcached_free(memc_);
    memc_ = nullptr;
  }
}

} // namespace rdd
