/*
 * Copyright (C) 2017, Yeolar
 */

#include <functional>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>

#include "raster/io/FSUtil.h"
#include "raster/thread/ThreadUtil.h"
#include "raster/util/Exception.h"
#include "raster/util/LogBase.h"
#include "raster/util/String.h"

namespace {

size_t getSize(int fd) {
  if (fd >= 0) {
    struct stat fs;
    if (fstat(fd, &fs) != -1) {
      return fs.st_size;
    }
  }
  return 0;
}

} // namespace

namespace rdd {
namespace logging {

namespace detail {

inline char getLevelLabel(int level) {
  return level >= 0 ? "DIWEF"[level] : 'V';
}

size_t writeLogHeader(char* buffer,
                      size_t size,
                      int level,
                      const char* file,
                      int line,
                      const char* traceid) {
  char* p = buffer;
  size_t n = size;
  ssize_t r;

  *p++ = '[';
  *p++ = getLevelLabel(level);
  n -= 2;

  uint64_t now = timestampNow();
  time_t t = now / 1000000;
  struct tm tm;
  ::localtime_r(&t, &tm);
  r = strftime(p, n, " %y%m%d %T", &tm);
  p += r;
  n -= r;

  int tid = osThreadId();
  r = snprintf(p, n, ".%06zu %5d %s:%d] ", now % 1000000, tid, file, line);
  p += r;
  n -= r;

  if (traceid) {
    r = snprintf(p, n, "trace(%s) ", traceid);
    p += r;
    n -= r;
  }
  return size - n;
}

} // namespace detail

BaseLogger::BaseLogger(const std::string& name)
  : name_(name),
    fd_(-1),
    level_(1),
    rotate_(0),
    splitSize_(0),
    async_(false) {
  handle_ = std::thread(&BaseLogger::run, this);
  setThreadName(handle_.native_handle(), "LogThread");
}

BaseLogger::~BaseLogger() {
  close();
  handle_.join();
}

void BaseLogger::run() {
  using namespace std::placeholders;

  while (true) {
    queue_.sweep(std::bind(&BaseLogger::write, this, _1));
    usleep(1000);
  }
}

void BaseLogger::log(std::string&& message, bool async) {
  if (async_ && async) {
    queue_.insertHead(std::move(message));
  } else {
    write(std::move(message));
  }
}

void BaseLogger::setLogFile(const std::string& file) {
  if (!file.empty() && file != file_) {
    file_ = file;

    std::lock_guard<std::mutex> guard(lock_);
    close();
    open();
  }
}

void BaseLogger::setLevel(int level) {
  level_ = level;
}

void BaseLogger::setRotate(int rotate, size_t size) {
  rotate_ = rotate;
  splitSize_ = size;
}

void BaseLogger::setAsync(bool async) {
  async_ = async;
}

void BaseLogger::setOptions(const Options& opts) {
  setLogFile(opts.logFile);
  setLevel(opts.level);
  setRotate(opts.rotate, opts.splitSize);
  setAsync(opts.async);
}

void BaseLogger::open() {
  // Failed to -1, will use stderr.
  fd_ = ::open(file_.c_str(), O_RDWR | O_APPEND | O_CREAT, 0666);
}

void BaseLogger::close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

void BaseLogger::split() {
  Path file(file_);
  for (auto& f : ls(file.parent())) {
    auto path = file.parent() / f;
    if (path.base() == file.name()) {
      int no = atoi(path.ext().c_str());
      if (rotate_ > 0 && no >= rotate_) {
        remove(path);
      } else if (no > 0) {
        rename(path, path.replaceExt(to<std::string>(no+1)));
      }
    }
  }

  std::lock_guard<std::mutex> guard(lock_);
  close();
  rename(file, file + ".1");
  open();
}

void BaseLogger::write(std::string&& message) {
  int fd;
  {
    std::lock_guard<std::mutex> guard(lock_);
    fd = fd_ >= 0 ? fd_ : STDERR_FILENO;
    ::write(fd, message.c_str(), message.size());
  }
  if (splitSize_ > 0 && getSize(File(fd)) >= splitSize_) {
    split();
  }
}

LogMessage::LogMessage(
    BaseLogger* logger,
    int level,
    const char* file,
    int line,
    const std::string& traceid)
  : out_(buf_, kBufSize)
  , logger_(logger)
  , level_(level)
  , errno_(errno) {
  out_.advance(
      detail::writeLogHeader(
          buf_, kBufSize, level, file, line,
          traceid.empty() ? nullptr : traceid.c_str()));
}

LogMessage::~LogMessage() {
  out_ << std::endl;
  logger_->log(out_.str(), level_ < LOG_ERROR);
  errno = errno_;
  if (level_ == LOG_FATAL) {
    abort();
  }
}

RawLogMessage::RawLogMessage(BaseLogger* logger)
  : out_(buf_, sizeof(buf_))
  , logger_(logger)
  , errno_(errno) {
}

RawLogMessage::~RawLogMessage() {
  out_ << std::endl;
  logger_->log(out_.str());
  errno = errno_;
}

} // namespace logging
} // namespace rdd
