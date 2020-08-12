/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <csignal>
#include <vector>

#include "raster/concurrency/Executor.h"

namespace raster {

void setupSignal(int signo, void (*handler)(int));
void setupSignal(int signo, void (*handler)(int, siginfo_t*, void*));

void setupIgnoreSignal(int signo);
void setupReloadSignal(int signo);
void setupShutdownSignal(int signo);

void sendSignal(int signo, const char* pidfile);

class Shutdown {
 public:
  Shutdown() {}

  void addTask(VoidFunc&& callback) {
    callbacks_.push_back(std::move(callback));
  }

  void run() {
    for (auto& cb : callbacks_) {
      cb();
    }
    exit(0);
  }

 private:
  std::vector<VoidFunc> callbacks_;
};

} // namespace raster
