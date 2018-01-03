/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <csignal>
#include <vector>

#include "raster/util/Function.h"

namespace rdd {

void setupSignal(int signo, void (*handler)(int));
void setupSignal(int signo, void (*handler)(int, siginfo_t*, void*));

void setupIgnoreSignal(int signo);
void setupReloadSignal(int signo);
void setupShutdownSignal(int signo);
void setupMemoryProtectSignal();

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

} // namespace rdd
