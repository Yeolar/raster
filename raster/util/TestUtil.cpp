/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/TestUtil.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <raster/io/File.h>
#include <raster/io/FileUtil.h>
#include <raster/util/Conv.h>
#include <raster/util/String.h>

namespace rdd {
namespace test {

namespace {

fs::path generateUniquePath(fs::path path, StringPiece namePrefix) {
  if (path.empty()) {
    path = fs::temp_directory_path();
  }
  if (namePrefix.empty()) {
    path /= fs::unique_path();
  } else {
    path /= fs::unique_path(
        to<std::string>(namePrefix, ".%%%%-%%%%-%%%%-%%%%"));
  }
  return path;
}

} // namespace

TemporaryFile::TemporaryFile(StringPiece namePrefix,
                             fs::path dir,
                             Scope scope,
                             bool closeOnDestruction)
  : scope_(scope),
    closeOnDestruction_(closeOnDestruction),
    fd_(-1),
    path_(generateUniquePath(std::move(dir), namePrefix)) {
  fd_ = open(path_.string().c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
  checkUnixError(fd_, "open failed");

  if (scope_ == Scope::UNLINK_IMMEDIATELY) {
    boost::system::error_code ec;
    fs::remove(path_, ec);
    if (ec) {
      RDDLOG(WARN) << "unlink on construction failed: " << ec;
    } else {
      path_.clear();
    }
  }
}

const fs::path& TemporaryFile::path() const {
  RDDCHECK_NE(scope_, Scope::UNLINK_IMMEDIATELY);
  DCHECK(!path_.empty());
  return path_;
}

TemporaryFile::~TemporaryFile() {
  if (fd_ != -1 && closeOnDestruction_) {
    if (close(fd_) == -1) {
      RDDPLOG(ERROR) << "close failed";
    }
  }

  // If we previously failed to unlink() (UNLINK_IMMEDIATELY), we'll
  // try again here.
  if (scope_ != Scope::PERMANENT && !path_.empty()) {
    boost::system::error_code ec;
    fs::remove(path_, ec);
    if (ec) {
      RDDLOG(WARN) << "unlink on destruction failed: " << ec;
    }
  }
}

TemporaryDirectory::TemporaryDirectory(StringPiece namePrefix,
                                       fs::path dir,
                                       Scope scope)
  : scope_(scope),
    path_(generateUniquePath(std::move(dir), namePrefix)) {
  fs::create_directory(path_);
}

TemporaryDirectory::~TemporaryDirectory() {
  if (scope_ == Scope::DELETE_ON_DESTRUCTION) {
    boost::system::error_code ec;
    fs::remove_all(path_, ec);
    if (ec) {
      RDDLOG(WARN) << "recursive delete on destruction failed: " << ec;
    }
  }
}

ChangeToTempDir::ChangeToTempDir() : initialPath_(fs::current_path()) {
  std::string p = dir_.path().string();
  ::chdir(p.c_str());
}

ChangeToTempDir::~ChangeToTempDir() {
  std::string p = initialPath_.string();
  ::chdir(p.c_str());
}

} // namespace test
} // namespace rdd
