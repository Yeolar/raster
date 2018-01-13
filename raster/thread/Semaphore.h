/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
