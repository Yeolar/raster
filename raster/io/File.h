/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include "raster/util/Exception.h"
#include "raster/util/Logging.h"
#include "raster/util/noncopyable.h"

namespace rdd {

class File : private noncopyable {
public:
  File() : fd_(-1), ownsFd_(false) {}

  // Takes ownership of the file descriptor if ownsFd is true
  explicit File(int fd, bool ownsFd = false)
    : fd_(fd), ownsFd_(ownsFd) {
    RDDCHECK(fd >= -1) << "fd must be -1 or non-negative";
    RDDCHECK(fd != -1 || !ownsFd) << "cannot own -1";
  }

  explicit File(const char* name,
                int flags = O_RDONLY,
                mode_t mode = 0666) {
    init(name, flags, mode);
  }

  explicit File(const std::string& name,
                int flags = O_RDONLY,
                mode_t mode = 0666) {
    init(name.c_str(), flags, mode);
  }

  ~File() {
    auto fd = fd_;
    if (!closeNoThrow()) {  // ignore most errors
      DCHECK(errno != EBADF)
        << "closing fd " << fd << ", it may already have been closed."
        << " Another time, this might close the wrong FD.";
    }
  }

  File(File&& other)
    : fd_(other.fd_), ownsFd_(other.ownsFd_) {
    other.release();
  }

  File& operator=(File&& other) {
    closeNoThrow();
    swap(other);
    return *this;
  }

  int fd() const { return fd_; }

  operator bool() const { return fd_ != -1; }

  void close() {
    if (!closeNoThrow()) {
      throwSystemError("close() failed");
    }
  }

  bool closeNoThrow() {
    int r = ownsFd_ ? ::close(fd_) : 0;
    release();
    return r == 0;
  }

  int release() {
    int released = fd_;
    fd_ = -1;
    ownsFd_ = false;
    return released;
  }

  void swap(File& other) {
    std::swap(fd_, other.fd_);
    std::swap(ownsFd_, other.ownsFd_);
  }

private:
  void init(const char* name, int flags, mode_t mode) {
    fd_ = ::open(name, flags, mode);
    ownsFd_ = false;
    if (fd_ == -1) {
      throwSystemError(to<std::string>("open ", name, " failed"));
    }
    ownsFd_ = true;
  }

  int fd_;
  bool ownsFd_;
};

inline void swap(File& a, File& b) { a.swap(b); }

} // namespace rdd
