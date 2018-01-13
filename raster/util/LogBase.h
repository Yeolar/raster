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

#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "raster/thread/AtomicLinkedList.h"
#include "raster/util/FixedStream.h"
#include "raster/util/Time.h"

#ifndef __FILENAME__
#define __FILENAME__ ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#endif

namespace rdd {
namespace logging {

enum LogSeverity {
  // framework
  LOG_V5      = -5, // coroutine
  LOG_V4      = -4, // data
  LOG_V3      = -3, // protocol, plugin
  LOG_V2      = -2, // event, task
  LOG_V1      = -1, // net, common
  // application
  LOG_DEBUG   = 0,
  LOG_INFO    = 1,
  LOG_WARNING = 2,
  LOG_WARN    = LOG_WARNING,
  LOG_ERROR   = 3,
  LOG_FATAL   = 4,
};

static constexpr size_t kBufSize = 4096;

class BaseLogger {
 public:
  struct Options {
    std::string logFile;
    int level;
    int rotate;
    size_t splitSize;
    bool async;
  };

  BaseLogger(const std::string& name);
  virtual ~BaseLogger();

  void log(std::string&& message, bool async = true);

  int level() const { return level_; }

  void setLogFile(const std::string& file);
  void setLevel(int level);
  void setRotate(int rotate, size_t size);
  void setAsync(bool async);
  void setOptions(const Options& opts);

  void run();

 private:
  void open();
  void close();

  void split();

  void write(std::string&& message);

  std::string name_;
  std::string file_;
  int fd_;
  int level_;
  int rotate_;
  size_t splitSize_;
  bool async_;

  std::thread handle_;
  std::mutex lock_;
  AtomicLinkedList<std::string> queue_;
};

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".
class LogMessageVoidify {
 public:
  LogMessageVoidify() {}
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) {}
};

class LogMessage {
 public:
  LogMessage(BaseLogger* logger,
             int level,
             const char* file,
             int line,
             const std::string& traceid = "");

  ~LogMessage();

  FixedOstream& stream() { return out_; }

 private:
  char buf_[kBufSize];
  FixedOstream out_;
  BaseLogger* logger_;
  int level_;
  int errno_;
};

class RawLogMessage {
 public:
  RawLogMessage(BaseLogger* logger);

  ~RawLogMessage();

  FixedOstream& stream() { return out_; }

 private:
  char buf_[kBufSize];
  FixedOstream out_;
  BaseLogger* logger_;
  int errno_;
};

namespace detail {

struct LogScopeParam {
  BaseLogger* logger;
  int level;
  const char* file;
  int line;
  uint64_t threshold;
};

} // namespace detail

template <typename F>
class CostLogMessage {
 public:
  CostLogMessage(detail::LogScopeParam&& param, F&& fn)
    : param_(std::move(param)), function_(std::move(fn)) {
  }

  ~CostLogMessage() {
    execute();
  }

  operator bool() const { return true; }

 private:
  void execute() {
    uint64_t start = timestampNow();
    function_();
    uint64_t cost = timePassed(start);

    if (cost >= param_.threshold) {
      LogMessage(param_.logger,
                 param_.level,
                 param_.file,
                 param_.line).stream()
        << "cost(" << cost / 1000.0 << "ms)";
    }
  }

  detail::LogScopeParam param_;
  F function_;
};

template <typename F>
bool operator+(detail::LogScopeParam&& param, F&& fn) {
  return CostLogMessage<typename std::decay<F>::type>(
      std::move(param), std::forward<F>(fn));
}

} // namespace logging
} // namespace rdd
