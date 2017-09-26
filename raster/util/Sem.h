/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <semaphore.h>
#include "raster/util/Exception.h"
#include "raster/util/noncopyable.h"

namespace rdd {

class Sem : noncopyable {
public:
  Sem(unsigned int value = 0) {
    checkUnixError(sem_init(&sem_, 0, value), "failed to init sem");
  }

  ~Sem() {
    checkUnixError(sem_destroy(&sem_), "failed to destroy sem");
  }

  void post() const {
    checkUnixError(sem_post(&sem_), "failed to post sem");
  }

  void wait() const {
    checkUnixError(sem_wait(&sem_), "failed to wait sem");
  }

  int getValue() const {
    int value = INT_MIN;
    checkUnixError(sem_getvalue(&sem_, &value), "failed to get sem value");
    return value;
  }

private:
  mutable sem_t sem_;
};

} // namespace rdd
