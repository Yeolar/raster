/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <fcntl.h>

#include "raster/util/Macro.h"

namespace rdd {

class File {
public:
  File() noexcept;

  // Takes ownership of the file descriptor if ownsFd is true
  explicit File(int fd, bool ownsFd = false) noexcept;

  explicit File(const char* name, int flags = O_RDONLY, mode_t mode = 0666);

  explicit File(
      const std::string& name, int flags = O_RDONLY, mode_t mode = 0666);

  ~File();

  static File temporary();

  int fd() const { return fd_; }

  operator bool() const { return fd_ != -1; }

  File dup() const;

  void close();
  bool closeNoThrow();

  int release() noexcept;

  void swap(File& other);

  File(File&& other) noexcept;
  File& operator=(File&& other);

  void allocate(size_t offset, size_t bytes);
  void truncate(size_t bytes);

  void fsync() const;
  void fdatasync() const;

  void lock();
  bool try_lock();
  void unlock();

  size_t getSize() const;

  NOCOPY(File);

private:
  int fd_;
  bool ownsFd_;
};

inline void swap(File& a, File& b) { a.swap(b); }

/**
 * Provides random access to a file.
 * This implementation currently works by mmaping the file and working from the
 * in-memory copy.
 */
class FileContents {
public:
  explicit FileContents(const File& file);

  ~FileContents();

  size_t getFileLength() { return fileLen_; }

  void copy(size_t offset, void* buf, size_t length);

  size_t copyPartial(size_t offset, void* buf, size_t maxLength);

  template <typename T = void>
  const T* get(size_t offset, size_t length) {
    return reinterpret_cast<const T*>(getHelper(offset, length));
  }

private:
  NOCOPY(FileContents);

  const void* getHelper(size_t offset, size_t length);

  File file_;
  size_t fileLen_;
  const void* map_;
};

}  // namespace rdd
