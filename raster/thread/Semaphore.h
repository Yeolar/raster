/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <semaphore.h>

#include "raster/util/Exception.h"

namespace rdd {

class Semaphore {
public:
  Semaphore(unsigned int value = 0) {
    checkUnixError(sem_init(&sem_, 0, value), "sem_init");
  }

  ~Semaphore() {
    checkUnixError(sem_destroy(&sem_), "sem_destroy");
  }

  void post() const {
    checkUnixError(sem_post(&sem_), "sem_post");
  }

  void wait() const {
    checkUnixError(sem_wait(&sem_), "sem_wait");
  }

  void trywait() const {
    checkUnixError(sem_trywait(&sem_), "sem_trywait");
  }

private:
  mutable sem_t sem_;
};

} // namespace rdd
