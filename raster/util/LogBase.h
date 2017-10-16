/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <atomic>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "raster/io/FSUtil.h"
#include "raster/util/Exception.h"
#include "raster/util/FixedStream.h"
#include "raster/util/String.h"
#include "raster/util/ThreadUtil.h"
#include "raster/util/Time.h"

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
  template <class T>
  class SwapQueue {
  public:
    SwapQueue() {}

    void add(const T& value) {
      std::lock_guard<std::mutex> guard(lock_);
      producer_.push_back(value);
    }

    void consume(const std::function<void(const T&)>& consumer) {
      {
        std::lock_guard<std::mutex> guard(lock_);
        std::swap(producer_, consumer_);
      }
      if (!consumer_.empty()) {
        for (auto& value : consumer_) {
          consumer(value);
        }
        consumer_.clear();
      }
    }

  private:
    std::vector<T> producer_;
    std::vector<T> consumer_;
    std::mutex lock_;
  };

public:
  struct Options {
    std::string logFile;
    int level;
    int rotate;
    int splitSize;
    bool async;
  };

  BaseLogger(const std::string& name)
    : name_(name)
    , fd_(-1) {
  }

  virtual ~BaseLogger() {
    close();
    if (async_) {
      handle_.join();
    }
  }

  void run() {
    using namespace std::placeholders;
    while (true) {
      queue_.consume(std::bind(&BaseLogger::write, this, _1));
      usleep(1000);
    }
  }

  void log(const std::string& message, bool async = true) {
    if (async_ && async) {
      queue_.add(message);
    } else {
      std::lock_guard<std::mutex> guard(lock_);
      write(message);
    }
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

  void setRotate(int rotate, int size) {
    if (size > 0) {
      rotate_ = rotate;
      splitSize_ = size;
    }
  }

  void setAsync(bool async) {
    async_ = async;
    if (async_) {
      handle_ = std::thread(&BaseLogger::run, this);
      setThreadName(handle_.native_handle(), "LogThread");
    }
  }

  void setOptions(const Options& opts) {
    setLogFile(opts.logFile);
    setLevel(opts.level);
    setRotate(opts.rotate, opts.splitSize);
    setAsync(opts.async);
  }

private:
  void open() {
    fd_ = ::open(file_.c_str(), O_RDWR | O_APPEND | O_CREAT, 0666);
  }

  void close() {
    if (fd_ >= 0) {
      ::close(fd_);
    }
  }

  int getSize() const {
    if (fd_ >= 0) {
      struct stat fs;
      if (fstat(fd_, &fs) != -1) {
        return fs.st_size;
      }
    }
    return -1;
  }

  void split() {
    close();
    fs::path file(file_);
    for (fs::directory_iterator it(file.parent_path());
         it != fs::directory_iterator(); ++it) {
      auto path = it->path();
      if (path.stem() == file.filename()) {
        int no = atoi(path.extension().c_str());
        if (rotate_ > 0 && no >= rotate_) {
          remove(path.c_str());
        } else if (no > 0) {
          rename(path.c_str(),
                 path.replace_extension(to<std::string>(no+1)).c_str());
        }
      }
    }
    rename(file_.c_str(), (file_ + ".1").c_str());
    open();
  }

  void write(const std::string& message) {
    int fd = fd_;
    fd = fd >= 0 ? fd : STDERR_FILENO;
    checkUnixError(::write(fd, message.c_str(), message.size()),
                   "write log error on fd=", fd);
    if (splitSize_ > 0 && getSize() >= splitSize_) {
      split();
    }
  }

  std::string name_;
  std::string file_;
  std::atomic<int> fd_;
  int level_{1};
  int rotate_{0};
  int splitSize_{0};
  bool async_{false};
  std::thread handle_;
  SwapQueue<std::string> queue_;
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
    logger_->log(out_.str(), level_ != LOG_FATAL);
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

} // namespace logging
} // namespace rdd
