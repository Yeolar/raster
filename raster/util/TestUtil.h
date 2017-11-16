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
                         fs::path dir = fs::path(),
                         Scope scope = Scope::UNLINK_ON_DESTRUCTION,
                         bool closeOnDestruction = true);
  ~TemporaryFile();

  // Movable, but not copiable
  TemporaryFile(TemporaryFile&&) = default;
  TemporaryFile& operator=(TemporaryFile&&) = default;

  int fd() const { return fd_; }
  const fs::path& path() const;

 private:
  Scope scope_;
  bool closeOnDestruction_;
  int fd_;
  fs::path path_;
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
                              fs::path dir = fs::path(),
                              Scope scope = Scope::DELETE_ON_DESTRUCTION);
  ~TemporaryDirectory();

  // Movable, but not copiable
  TemporaryDirectory(TemporaryDirectory&&) = default;
  TemporaryDirectory& operator=(TemporaryDirectory&&) = default;

  const fs::path& path() const { return path_; }

 private:
  Scope scope_;
  fs::path path_;
};

/**
 * Changes into a temporary directory, and deletes it with all its contents
 * upon destruction, also changing back to the original working directory.
 */
class ChangeToTempDir {
public:
  ChangeToTempDir();
  ~ChangeToTempDir();

  // Movable, but not copiable
  ChangeToTempDir(ChangeToTempDir&&) = default;
  ChangeToTempDir& operator=(ChangeToTempDir&&) = default;

  const fs::path& path() const { return dir_.path(); }

private:
  fs::path initialPath_;
  TemporaryDirectory dir_;
};

} // namespace test
} // namespace rdd
