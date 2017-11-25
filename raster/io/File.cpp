/*
* Copyright 2017 Facebook, Inc.
* Copyright (C) 2017, Yeolar
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "raster/io/File.h"
#include "raster/io/FileUtil.h"
#include "raster/util/Exception.h"
#include "raster/util/Logging.h"

namespace rdd {

File::File() noexcept : fd_(-1), ownsFd_(false) {}

File::File(int fd, bool ownsFd) noexcept
  : fd_(fd), ownsFd_(ownsFd) {
  RDDCHECK(fd >= -1) << "fd must be -1 or non-negative";
  RDDCHECK(fd != -1 || !ownsFd) << "cannot own -1";
}

File::File(const char* name, int flags, mode_t mode)
  : fd_(::open(name, flags, mode))
  , ownsFd_(false) {
  if (fd_ == -1) {
    throwSystemError("open(", name, ") failed");
  }
  ownsFd_ = true;
}

File::File(const std::string& name, int flags, mode_t mode)
  : File(name.c_str(), flags, mode) {}

File::File(File&& other) noexcept
  : fd_(other.fd_), ownsFd_(other.ownsFd_) {
  other.release();
}

File& File::operator=(File&& other) {
  closeNoThrow();
  swap(other);
  return *this;
}

File::~File() {
  auto fd = fd_;
  if (!closeNoThrow()) {  // ignore most errors
    DCHECK(errno != EBADF)
      << "closing fd " << fd << ", it may already have been closed."
      << " Another time, this might close the wrong FD.";
  }
}

File File::temporary() {
  FILE* tmpFile = tmpfile();
  checkFopenError(tmpFile, "tmpfile() failed");
  SCOPE_EXIT { fclose(tmpFile); };

  int fd = ::dup(fileno(tmpFile));
  checkUnixError(fd, "dup() failed");
  return File(fd, true);
}

File File::dup() const {
  if (fd_ != -1) {
    int fd = ::dup(fd_);
    checkUnixError(fd, "dup() failed");
    return File(fd, true);
  }
  return File();
}

void File::close() {
  if (!closeNoThrow()) {
    throwSystemError("close() failed");
  }
}

bool File::closeNoThrow() {
  int r = ownsFd_ ? ::close(fd_) : 0;
  release();
  return r == 0;
}

int File::release() noexcept {
  int released = fd_;
  fd_ = -1;
  ownsFd_ = false;
  return released;
}

void File::swap(File& other) {
  std::swap(fd_, other.fd_);
  std::swap(ownsFd_, other.ownsFd_);
}

void File::allocate(size_t offset, size_t bytes) {
  int r = ::posix_fallocate(fd_, to<off_t>(offset), to<off_t>(bytes));
  checkPosixError(r, "posix_fallocate() bytes [",
                  offset, ", ", offset + bytes, ") failed");
}

void File::truncate(size_t bytes) {
  checkUnixError(::ftruncate(fd_, to<off_t>(bytes)), "ftruncate() failed");
}

void File::fsync() const {
  checkUnixError(::fsync(fd_), "fsync() failed");
}

void File::fdatasync() const {
  checkUnixError(::fdatasync(fd_), "fdatasync() failed");
}

void File::lock() {
  checkUnixError(flockNoInt(fd_, LOCK_EX), "flock() failed (lock)");
}

bool File::try_lock() {
  int r = flockNoInt(fd_, LOCK_EX | LOCK_NB);
  // flock returns EWOULDBLOCK if already locked
  if (r == -1 && errno == EWOULDBLOCK) return false;
  checkUnixError(r, "flock() failed (try_lock)");
  return true;
}

void File::unlock() {
  checkUnixError(flockNoInt(fd_, LOCK_UN), "flock() failed (unlock)");
}

size_t File::getSize() const {
  struct stat stat;
  checkUnixError(fstat(fd_, &stat), "stat failed");
  return to<size_t>(stat.st_size);
}

FileContents::FileContents(const File& origFile)
    : file_(origFile.dup()), fileLen_(file_.getSize()), map_(nullptr) {
  // len of 0 for empty files results in invalid argument
  if (fileLen_ > 0) {
    map_ = mmap(nullptr, fileLen_, PROT_READ, MAP_SHARED, file_.fd(), 0);
    if (map_ == MAP_FAILED) {
      throwSystemError("mmap() failed");
    }
  }
}

FileContents::~FileContents() {
  if (map_) {
    munmap(const_cast<void*>(map_), fileLen_);
  }
}

void FileContents::copy(size_t offset, void* buf, size_t length) {
  if (copyPartial(offset, buf, length) != length) {
    RDDLOG(FATAL) << "File too short or corrupt";
  }
}

size_t FileContents::copyPartial(size_t offset, void* buf, size_t maxLength) {
  if (offset >= fileLen_)
    return 0;
  size_t length = std::min(fileLen_ - offset, maxLength);
  memcpy(buf, static_cast<const char*>(map_) + offset, length);
  return length;
}

const void* FileContents::getHelper(size_t offset, size_t length) {
  if (length != 0 && offset + length > fileLen_) {
    RDDLOG(FATAL) << "File too short or corrupt";
  }
  return static_cast<const char*>(map_) + offset;
}

}  // namespace rdd
