/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/coroutine/GenericExecutor.h"
#include "raster/coroutine/FiberManager.h"
#include "raster/net/AsyncClient.h"

namespace rdd {

class AsyncClientExecutor : public GenericExecutor {
public:
  AsyncClientExecutor(AsyncClient* client) : client_(client) {
    assert(client_->callbackMode());
  }

  virtual ~AsyncClientExecutor() {}

  void handle() {
    FiberManager::yield();
    client_->callback();
  }

  AsyncClient* client() const { return client_.get(); }

private:
  std::shared_ptr<AsyncClient> client_;
};

} // namespace rdd
