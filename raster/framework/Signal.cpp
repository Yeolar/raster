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

#include "raster/framework/Signal.h"

#include "raster/framework/Config.h"
#include "accelerator/Backtrace.h"
#include "accelerator/Exception.h"
#include "accelerator/Logging.h"
#include "accelerator/MemoryProtect.h"
#include "accelerator/ProcessUtil.h"
#include "accelerator/Singleton.h"

namespace rdd {

static void ignoreSignalHandler(int signo) {
  ACCLOG(INFO) << "signal '" << strsignal(signo) << "' received, ignore it";
}

static void reloadSignalHandler(int signo) {
  ACCLOG(INFO) << "signal '" << strsignal(signo) << "' received, reload";
  acc::Singleton<ConfigManager>::get()->load();
}

static void shutdownSignalHandler(int signo) {
  ACCLOG(INFO) << "signal '" << strsignal(signo) << "' received, exit...";
  acc::Singleton<Shutdown>::get()->run();
}

static void memoryProtectSignalHandler(int signo, siginfo_t* info, void*) {
  std::array<char, 128> buffer;
  int n = snprintf(buffer.data(), buffer.size(),
                   "Segmentation fault (sig=%d), fault address: %p.\n",
                   signo, info->si_addr);
  n = ::write(STDERR_FILENO, buffer.data(), n);
  acc::recordBacktrace();
  n = ::write(STDERR_FILENO, "\n", 1);
  acc::MemoryProtect(info->si_addr).unprotect();
}

void setupSignal(int signo, void (*handler)(int)) {
  struct sigaction sa;
  sa.sa_handler = handler;
  if (sigemptyset(&sa.sa_mask) == -1 ||
      sigaction(signo, &sa, nullptr) == -1) {
    ACCLOG(FATAL) << "signal '" << strsignal(signo) << "' set handler";
  }
}

void setupSignal(int signo, void (*handler)(int, siginfo_t*, void*)) {
  struct sigaction sa;
  sa.sa_sigaction = handler;
  sa.sa_flags = SA_SIGINFO;
  if (sigemptyset(&sa.sa_mask) == -1 ||
      sigaction(signo, &sa, nullptr) == -1) {
    ACCLOG(FATAL) << "signal '" << strsignal(signo) << "' set handler";
  }
}

void setupIgnoreSignal(int signo) {
  setupSignal(signo, ignoreSignalHandler);
}

void setupReloadSignal(int signo) {
  setupSignal(signo, reloadSignalHandler);
}

void setupShutdownSignal(int signo) {
  setupSignal(signo, shutdownSignalHandler);
}

void setupMemoryProtectSignal() {
  setupSignal(SIGSEGV, memoryProtectSignalHandler);
}

void sendSignal(int signo, const char* pidfile) {
  acc::checkUnixError(kill(acc::readPid(pidfile), signo));
}

} // namespace rdd
