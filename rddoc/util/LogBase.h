/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rddoc/util/FixedStream.h"
#include "rddoc/util/String.h"
#include "rddoc/util/ThreadUtil.h"
#include "rddoc/util/Time.h"

#ifndef __FILENAME__
#define __FILENAME__ ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#endif

namespace rdd {
namespace logging {

enum LogSeverity {
  // framework
  LOG_V5      = -5, // coroutine context
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

const size_t BUF_SIZE = 4096;

inline char getShortLevelName(int level) {
  return level >= 0 ? "DIWEF"[level] : 'V';
}

inline const char* getLevelName(int level) {
  const char* names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
  return level >= 0 ? names[level] : "VERB";
}

class BaseLogger {
public:
  struct Options {
    std::string logfile;
    int level;
  };

  BaseLogger(const std::string& name)
    : time_(time(nullptr))
    , name_(name)
    , level_(1)
    , fd_(-1) {
  }

  virtual ~BaseLogger() {
    close();
  }

  void log(const std::string& message) {
    std::lock_guard<std::mutex> guard(lock_);
    ::write(fd_ >= 0 ? fd_ : STDERR_FILENO, message.c_str(), message.size());
  }

  int level() const { return level_; }
  void setLevel(int level) { level_ = level; }

  void setLogFile(const std::string& file) {
    if (!file.empty() && file != file_) {
      file_ = file;
      close();
      open();
    }
  }

  void setOptions(const Options& opts) {
    setLogFile(opts.logfile);
    setLevel(opts.level);
  }

protected:
  time_t time_;

private:
  void open() {
    fd_ = ::open(file_.c_str(), O_RDWR | O_APPEND | O_CREAT, 0666);
  }

  void close() {
    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

  std::string name_;
  std::string file_;
  int level_;
  int fd_;
  std::mutex lock_;
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
  LogMessage(BaseLogger* logger, int level, const char* file, int line,
             const std::string& traceid = "")
    : out_(buf_, sizeof(buf_))
    , logger_(logger)
    , level_(level)
    , errno_(errno) {
    uint64_t now = timestampNow();
    out_ << "[" << logging::getShortLevelName(level)
         << timePrintf(now / 1000000, " %y%m%d %T")
         << stringPrintf(".%06zu ", now % 1000000)
         << std::setw(5) << localThreadId() << " "
         << (traceid.empty() ? "" : traceid + " ")
         << file << ":" << line << "] ";
  }

  virtual ~LogMessage() {
    out_ << std::endl;
    logger_->log(out_.str());
    errno = errno_;
    if (level_ == LOG_FATAL)
      abort();
  }

  FixedOstream& stream() { return out_; }

private:
  char buf_[logging::BUF_SIZE];
  FixedOstream out_;
  BaseLogger* logger_;
  int level_;
  int errno_;
};

class RawLogMessage {
public:
  RawLogMessage(BaseLogger* logger)
    : out_(buf_, sizeof(buf_))
    , logger_(logger)
    , errno_(errno) {
  }

  virtual ~RawLogMessage() {
    out_ << std::endl;
    logger_->log(out_.str());
    errno = errno_;
  }

  FixedOstream& stream() { return out_; }

private:
  char buf_[logging::BUF_SIZE];
  FixedOstream out_;
  BaseLogger* logger_;
  int errno_;
};

}
}

