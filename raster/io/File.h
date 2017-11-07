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
#include "raster/io/FileUtil.h"
#include "raster/util/Exception.h"
#include "raster/util/Logging.h"

namespace rdd {

class File {
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

  void lock() {
    checkUnixError(flockNoInt(fd_, LOCK_EX), "flock() failed (lock)");
  }

  bool try_lock() {
    int r = flockNoInt(fd_, LOCK_EX | LOCK_NB);
    // flock returns EWOULDBLOCK if already locked
    if (r == -1 && errno == EWOULDBLOCK) return false;
    checkUnixError(r, "flock() failed (try_lock)");
    return true;
  }

  void unlock() {
    checkUnixError(flockNoInt(fd_, LOCK_UN), "flock() failed (unlock)");
  }

  ssize_t read(void* buf, size_t n) const {
    ssize_t r = readFull(fd_, buf, n);
    checkUnixError(r, "read failed");
    return r;
  }

  template <class T>
  ssize_t read(T& value) const {
    return read(&value, sizeof(T));
  }

  ssize_t write(const void* buf, size_t n) const {
    ssize_t r = writeFull(fd_, buf, n);
    checkUnixError(r, "write failed");
    return r;
  }

  template <class T>
  ssize_t write(T& value) const {
    return write(&value, sizeof(T));
  }

  off_t seek(off_t offset, int whence) const {
    off_t r = lseek(fd_, offset, whence);
    checkUnixError(r, "seek failed");
    return r;
  }

  NOCOPY(File);

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
