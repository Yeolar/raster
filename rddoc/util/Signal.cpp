/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/Signal.h"
#include <signal.h>
#include <string.h>
#include "rddoc/util/Logging.h"
#include "rddoc/util/Singleton.h"

namespace rdd {

static void ignoreSignalHandler(int signo) {
  RDDLOG(INFO) << "signal '" << strsignal(signo) << "' received, ignore it";
}

static void shutdownSignalHandler(int signo) {
  RDDLOG(INFO) << "signal '" << strsignal(signo) << "' received, exit...";
  Singleton<Shutdown>::get()->run();
}

void setupSignal(int signo, void (*handler)(int)) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = handler;
  if (sigemptyset(&sa.sa_mask) == -1 ||
      sigaction(signo, &sa, nullptr) == -1) {
    RDDLOG(FATAL) << "signal '" << strsignal(signo) << "' set sa_handler";
  }
}

void setupIgnoreSignal(int signo) {
  setupSignal(signo, ignoreSignalHandler);
}

void setupShutdownSignal(int signo) {
  setupSignal(signo, shutdownSignalHandler);
}

} // namespace rdd
