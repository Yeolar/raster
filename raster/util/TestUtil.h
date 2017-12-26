/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>

#include "raster/io/FSUtil.h"
#include "raster/util/Range.h"

namespace rdd {
namespace test {

/**
 * Temporary file.
 *
 * By default, the file is created in a system-specific location (the value
 * of the TMPDIR environment variable, or /tmp), but you can override that
 * with a different (non-empty) directory passed to the constructor.
 *
 * By default, the file is closed and deleted when the TemporaryFile object
 * is destroyed, but both these behaviors can be overridden with arguments
 * to the constructor.
 */
class TemporaryFile {
 public:
  enum class Scope {
    PERMANENT,
    UNLINK_IMMEDIATELY,
    UNLINK_ON_DESTRUCTION
  };
  explicit TemporaryFile(StringPiece namePrefix = StringPiece(),
                         Path dir = Path(),
                         Scope scope = Scope::UNLINK_ON_DESTRUCTION,
                         bool closeOnDestruction = true);
  ~TemporaryFile();

  TemporaryFile(const TemporaryFile&) = delete;
  TemporaryFile& operator=(const TemporaryFile&) = delete;

  int fd() const { return fd_; }
  const Path& path() const;

 private:
  Scope scope_;
  bool closeOnDestruction_;
  int fd_;
  Path path_;
};

/**
 * Temporary directory.
 *
 * By default, the temporary directory is created in a system-specific
 * location (the value of the TMPDIR environment variable, or /tmp), but you
 * can override that with a non-empty directory passed to the constructor.
 *
 * By default, the directory is recursively deleted when the TemporaryDirectory
 * object is destroyed, but that can be overridden with an argument
 * to the constructor.
 */

class TemporaryDirectory {
 public:
  enum class Scope {
    PERMANENT,
    DELETE_ON_DESTRUCTION
  };
  explicit TemporaryDirectory(StringPiece namePrefix = StringPiece(),
                              Path dir = Path(),
                              Scope scope = Scope::DELETE_ON_DESTRUCTION);
  ~TemporaryDirectory();

  TemporaryDirectory(const TemporaryDirectory&) = delete;
  TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

  const Path& path() const { return path_; }

 private:
  Scope scope_;
  Path path_;
};

/**
 * Changes into a temporary directory, and deletes it with all its contents
 * upon destruction, also changing back to the original working directory.
 */
class ChangeToTempDir {
 public:
  ChangeToTempDir();
  ~ChangeToTempDir();

  ChangeToTempDir(const ChangeToTempDir&) = delete;
  ChangeToTempDir& operator=(const ChangeToTempDir&) = delete;

  const Path& path() const { return dir_.path(); }

 private:
  Path initialPath_;
  TemporaryDirectory dir_;
};

} // namespace test
} // namespace rdd
