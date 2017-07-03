/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <semaphore.h>
#include "rddoc/util/Exception.h"
#include "rddoc/util/noncopyable.h"

namespace rdd {

class Sem : noncopyable {
public:
  Sem() {
    checkUnixError(sem_init(&sem_, 0, 0), "failed to init sem");
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

private:
  mutable sem_t sem_;
};

}

