/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/FSUtil.h"

#include <cassert>
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "raster/io/FileUtil.h"
#include "raster/util/Exception.h"

namespace rdd {

Path currentPath() {
  size_t n = PATH_MAX;

  while (true) {
    char buf[n];
    char* p = ::getcwd(buf, NELEMS(buf));
    if (p) {
      return Path(p);
    } else if (errno == ERANGE) {
      n *= 2;
    } else {
      throwSystemError("getcwd() failed");
    }
  }
  // not reached
}

Path absolute(const Path& path, const Path& base) {
  return path.isAbsolute() ? path : base / path;
}

Path canonical(const Path& path, const Path& base) {
  Path p = absolute(path, base);
  if (!p.isLink()) {
    return p;
  }

  Path parent = p.parent();
  ssize_t n = PATH_MAX;

  while (true) {
    char buf[n];
    ssize_t r = ::readlink(p.c_str(), buf, NELEMS(buf));
    checkUnixError(r, "readlink(", p, ") failed");
    if (r < n) {
      return parent / StringPiece(buf, r);
    }
    n *= 2;
  }
  // not reached
}

std::vector<Path> lsHelper(DIR* dir, const Path& path = "") {
  if (dir == nullptr) {
    throwSystemError("could not list contents",
                     path.empty() ? "" : " of ", path);
  }

  // If dir was opened with fdopendir and was read from previously, this is
  // needed to rewind the directory, at least on eglibc v2.13.
  ::rewinddir(dir);

  std::vector<Path> contents;

  while (true) {
    struct dirent entry;
    struct dirent* entryp;
    checkPosixError(::readdir_r(dir, &entry, &entryp),
                    "readdir(", path, ") failed");
    if (entryp == nullptr) {  // no more entries
      break;
    }
    Path name = entry.d_name;
    if (name == "." || name == "..") {
      continue;
    }
    contents.push_back(std::move(name));
  }

  ::closedir(dir);

  return contents;
}

std::vector<Path> ls(const Path& path) {
  return lsHelper(::opendir(path.c_str()), path);
}

std::vector<Path> ls(const File& dir) {
  return lsHelper(::fdopendir(dir.dup().release()));
}

void createDirectory(const Path& path) {
  assert(!path.empty());
  int r = ::mkdir(path.c_str(), 0755);
  if (r == 0) {
    syncDirectory(path.parent());
  } else {
    if (errno != EEXIST) {
      throwSystemError("mkdir(", path, ") failed");
    }
  }
}

void createDirectory(const File& dir, const Path& child) {
  assert(!child.isAbsolute());
  int r = ::mkdirat(dir.fd(), child.c_str(), 0755);
  if (r == 0) {
    fsync(dir);
  } else {
    if (errno != EEXIST) {
      throwSystemError("mkdir(", child, ") failed");
    }
  }
}

File openDirectory(const Path& path) {
  assert(!path.empty());
  int r = ::mkdir(path.c_str(), 0755);
  if (r == 0) {
    syncDirectory(path.parent());
  } else {
    if (errno != EEXIST) {
      throwSystemError("mkdir(", path, ") failed");
    }
  }
  // It'd be awesome if one could do O_RDONLY|O_CREAT|O_DIRECTORY here,
  // but at least on eglibc v2.13, this combination of flags creates a
  // regular file!
  int fd = openNoInt(path.c_str(), O_RDONLY | O_DIRECTORY);
  checkUnixError(fd, "open(", path, ") failed");
  return File(fd, true);
}

File openDirectory(const File& dir, const Path& child) {
  assert(!child.isAbsolute());
  int r = ::mkdirat(dir.fd(), child.c_str(), 0755);
  if (r == 0) {
    fsync(dir);
  } else {
    if (errno != EEXIST) {
      throwSystemError("mkdir(", child, ") failed");
    }
  }
  // It'd be awesome if one could do O_RDONLY|O_CREAT|O_DIRECTORY here,
  // but at least on eglibc v2.13, this combination of flags creates a
  // regular file!
  int fd = openatNoInt(dir.fd(), child.c_str(), O_RDONLY | O_DIRECTORY);
  checkUnixError(fd, "open(", child, ") failed");
  return File(fd, true);
}

File openFile(const File& dir, const Path& child, int flags) {
  assert(!child.isAbsolute());
  int fd = openatNoInt(dir.fd(), child.c_str(), flags, 0644);
  checkUnixError(fd, "open(", child, ") failed");
  return File(fd, true);
}

File tryOpenFile(const File& dir, const Path& child, int flags) {
  assert(!child.isAbsolute());
  int fd = openatNoInt(dir.fd(), child.c_str(), flags, 0644);
  if (fd == -1) {
    if (errno == EEXIST || errno == ENOENT) {
      return File();
    }
    throwSystemError("open(", child, ") failed");
  }
  return File(fd, true);
}

void remove(const Path& path) {
  while (true) {
    if (::remove(path.c_str()) == 0) {
      return;
    }
    if (errno == ENOENT) {
      return;
    } else if (errno == EEXIST || errno == ENOTEMPTY) {
      std::vector<Path> children = ls(path);
      for (auto& child : children) {
        remove(path / child);
      }
    } else {
      throwSystemError("remove(", path, ") failed");
    }
  }
}

void removeFile(const File& dir, const Path& path) {
  assert(!path.isAbsolute());
  if (::unlinkat(dir.fd(), path.c_str(), 0) == 0) {
    return;
  }
  if (errno == ENOENT) {
    return;
  }
  throwSystemError("remove(", path, ") failed");
}

void rename(const Path& oldPath, const Path& newPath) {
  checkUnixError(::rename(oldPath.c_str(), newPath.c_str()),
                 "rename ", oldPath, " to ", newPath, " failed");
}

void rename(const File& oldDir,
            const Path& oldChild,
            const File& newDir,
            const Path& newChild) {
  assert(!oldChild.isAbsolute());
  assert(!newChild.isAbsolute());
  checkUnixError(::renameat(oldDir.fd(), oldChild.c_str(),
                            newDir.fd(), newChild.c_str()),
                 "rename ", oldChild, " to ", newChild, " failed");
}

void syncDirectory(const Path& path) {
  int fd = openNoInt(path.c_str(), O_RDONLY);
  checkUnixError(fd, "open(", path, ") failed");
  checkUnixError(fsyncNoInt(fd), "fsync ", path, " failed");
  close(fd);
}

Path tempDirectoryPath() {
  const char* val = nullptr;

  (val = std::getenv("TMPDIR" )) ||
  (val = std::getenv("TMP"    )) ||
  (val = std::getenv("TEMP"   )) ||
  (val = std::getenv("TEMPDIR"));

  Path p(val ? val : "/tmp");

  if (p.empty() || !p.isDirectory()) {
    throwSystemErrorExplicit(ENOTDIR, "temp directory not found");
  }
  return p;
}

Path uniquePath(const std::string& model) {
  std::string s(model);
  const char hex[] = "0123456789abcdef";
  const int maxNibbles = 32;
  char ran[16];

  int nibblesUsed = maxNibbles;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%') {
      if (nibblesUsed == maxNibbles) {
        Random::secureRandom(ran, sizeof(ran));
        nibblesUsed = 0;
      }
      int c = ran[nibblesUsed / 2];
      c >>= 4 * (nibblesUsed++ & 1);
      s[i] = hex[c & 0xf];
    }
  }
  return s;
}

Path generateUniquePath(Path path, StringPiece namePrefix) {
  if (path.empty()) {
    path = tempDirectoryPath();
  }
  if (namePrefix.empty()) {
    path /= uniquePath();
  } else {
    path /= uniquePath(to<std::string>(namePrefix, "-%%%%%%%%%%%%%%%%"));
  }
  return path;
}

} // namespace rdd
