/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Signal.h"
#include <signal.h>
#include <string.h>
#include "raster/util/Backtrace.h"
#include "raster/util/Logging.h"
#include "raster/util/MemoryProtect.h"
#include "raster/util/Singleton.h"

namespace rdd {

static void ignoreSignalHandler(int signo) {
  RDDLOG(INFO) << "signal '" << strsignal(signo) << "' received, ignore it";
}

static void shutdownSignalHandler(int signo) {
  RDDLOG(INFO) << "signal '" << strsignal(signo) << "' received, exit...";
  Singleton<Shutdown>::get()->run();
}

static void sigsegvSignalHandler(int signo, siginfo_t* info, void* context) {
  std::array<char, 128> buffer;
  size_t n = snprintf(buffer.data(), buffer.size(),
                      "Segmentation fault (sig=%d), fault address: %p.\n",
                      signo, info->si_addr);
  ::write(STDERR_FILENO, buffer.data(), n);
  recordBacktrace();
  MemoryProtect(info->si_addr).unprotect();
}

void setupSignal(int signo, void (*handler)(int)) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = handler;
  if (sigemptyset(&sa.sa_mask) == -1 ||
      sigaction(signo, &sa, nullptr) == -1) {
    RDDLOG(FATAL) << "signal '" << strsignal(signo) << "' set handler";
  }
}

void setupSignal(int signo, void (*handler)(int, siginfo_t*, void*)) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = handler;
  sa.sa_flags = SA_SIGINFO;
  if (sigemptyset(&sa.sa_mask) == -1 ||
      sigaction(signo, &sa, nullptr) == -1) {
    RDDLOG(FATAL) << "signal '" << strsignal(signo) << "' set handler";
  }
}

void setupIgnoreSignal(int signo) {
  setupSignal(signo, ignoreSignalHandler);
}

void setupShutdownSignal(int signo) {
  setupSignal(signo, shutdownSignalHandler);
}

void setupSigsegvSignal() {
  setupSignal(SIGSEGV, sigsegvSignalHandler);
}

} // namespace rdd
