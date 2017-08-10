/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cassert>
#include <limits>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "raster/util/Conv.h"
#include "raster/util/ScopeGuard.h"

namespace rdd {

inline bool setCloseExec(int fd) {
  int r = fcntl(fd, F_GETFD, 0);
  if (r == -1) return false;
  return fcntl(fd, F_SETFD, r | FD_CLOEXEC) != -1;
}

inline bool setNonBlocking(int fd) {
  int r = fcntl(fd, F_GETFL, 0);
  if (r == -1) return false;
  return fcntl(fd, F_SETFL, r | O_NONBLOCK) != -1;
}

namespace detail {

template<class F, class... Args>
ssize_t wrapNoInt(F f, Args... args) {
  ssize_t r;
  do {
    r = f(args...);
  } while (r == -1 && errno == EINTR);
  return r;
}

inline void incr(ssize_t n) {}
inline void incr(ssize_t n, off_t& offset) { offset += n; }

template <class F, class... Offset>
ssize_t wrapFull(F f, int fd, void* buf, size_t count, Offset... offset) {
  char* b = static_cast<char*>(buf);
  ssize_t totalBytes = 0;
  ssize_t r;
  do {
    r = f(fd, b, count, offset...);
    if (r == -1) {
      if (errno == EINTR) {
        continue;
      }
      return r;
    }

    totalBytes += r;
    b += r;
    count -= r;
    incr(r, offset...);
  } while (r != 0 && count);  // 0 means EOF

  return totalBytes;
}

} // namespace detail

inline int openNoInt(const char* name, int flags, mode_t mode = 0666) {
  return int(detail::wrapNoInt(open, name, flags, mode));
}

inline int closeNoInt(int fd) {
  int r = close(fd);
  if (r == -1 && errno == EINTR) {
    r = 0;
  }
  return r;
}

inline ssize_t readNoInt(int fd, void* buf, size_t count) {
  return detail::wrapNoInt(read, fd, buf, count);
}

inline ssize_t writeNoInt(int fd, const void* buf, size_t count) {
  return detail::wrapNoInt(write, fd, buf, count);
}

inline ssize_t readFull(int fd, void* buf, size_t n) {
  return detail::wrapFull(read, fd, buf, n);
}

inline ssize_t writeFull(int fd, const void* buf, size_t n) {
  return detail::wrapFull(write, fd, const_cast<void*>(buf), n);
}

inline bool readBlock(int fd, void* buf, size_t n) {
  return readFull(fd, buf, n) == (ssize_t)n;
}

inline bool writeBlock(int fd, void* buf, size_t n) {
  return writeFull(fd, buf, n) == (ssize_t)n;
}

template <class Container>
bool readFile(const char* file_name, Container& out,
              size_t num_bytes = std::numeric_limits<size_t>::max()) {
  static_assert(sizeof(out[0]) == 1,
                "readFile: only containers with byte-sized elements accepted");
  assert(file_name);

  const auto fd = openNoInt(file_name, O_RDONLY);
  if (fd == -1) return false;

  size_t soFar = 0;
  SCOPE_EXIT {
    assert(out.size() >= soFar);
    out.resize(soFar);
    closeNoInt(fd);
  };

  struct stat buf;
  if (fstat(fd, &buf) == -1) return false;
  // Some files (notably under /proc and /sys on Linux) lie about
  // their size, so treat the size advertised by fstat under advise
  // but don't rely on it. In particular, if the size is zero, we
  // should attempt to read stuff. If not zero, we'll attempt to read
  // one extra byte.
  constexpr size_t initialAlloc = 1024 * 4;
  out.resize(
    std::min(
      buf.st_size > 0 ? size_t(buf.st_size + 1) : initialAlloc,
      num_bytes));

  while (soFar < out.size()) {
    const auto actual = readFull(fd, &out[soFar], out.size() - soFar);
    if (actual == -1) {
      return false;
    }
    soFar += actual;
    if (soFar < out.size()) {
      // File exhausted
      break;
    }
    out.resize(std::min(out.size() * 3 / 2, num_bytes));
  }

  return true;
}

template <class Container>
bool writeFile(const Container& data, const char* filename,
               mode_t mode = 0666,
               int flags = O_WRONLY | O_CREAT | O_TRUNC) {
  int fd = open(filename, flags, mode);
  if (fd == -1) {
    return false;
  }
  bool ok = data.empty() ||
    writeFull(fd, &data[0], data.size()) == static_cast<ssize_t>(data.size());
  return closeNoInt(fd) == 0 && ok;
}

} // namespace rdd
