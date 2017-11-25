/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cctype>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <system_error>
#include <boost/regex.hpp>

#include "raster/gen/Base.h"
#include "raster/gen/File.h"
#include "raster/gen/String.h"
#include "raster/io/HugePages.h"
#include "raster/util/Conv.h"
#include "raster/util/Range.h"

namespace rdd {

namespace {

// Get the default huge page size
size_t getDefaultHugePageSize() {
  // We need to parse /proc/meminfo
  static const boost::regex regex(R"!(Hugepagesize:\s*(\d+)\s*kB)!");
  size_t pageSize = 0;
  boost::cmatch match;

  bool error = gen::byLine("/proc/meminfo") |
    [&] (StringPiece line) -> bool {
      if (boost::regex_match(line.begin(), line.end(), match, regex)) {
        StringPiece numStr(
            line.begin() + match.position(1), size_t(match.length(1)));
        pageSize = to<size_t>(numStr) * 1024;  // in KiB
        return false;  // stop
      }
      return true;
    };

  if (error) {
    throw std::runtime_error("Can't find default huge page size");
  }
  return pageSize;
}

// Get raw huge page sizes (without mount points, they'll be filled later)
HugePageSizeVec readRawHugePageSizes() {
  // We need to parse file names from /sys/kernel/mm/hugepages
  static const boost::regex regex(R"!(hugepages-(\d+)kB)!");
  boost::smatch match;
  HugePageSizeVec vec;
  Path path("/sys/kernel/mm/hugepages");
  for (auto& p : ls(path)) {
    std::string filename(p.str());
    if (boost::regex_match(filename, match, regex)) {
      StringPiece numStr(
          filename.data() + match.position(1), size_t(match.length(1)));
      vec.emplace_back(to<size_t>(numStr) * 1024);
    }
  }
  return vec;
}

// Parse the value of a pagesize mount option
// Format: number, optional K/M/G/T suffix, trailing junk allowed
size_t parsePageSizeValue(StringPiece value) {
  static const boost::regex regex(R"!((\d+)([kmgt])?.*)!", boost::regex::icase);
  boost::cmatch match;
  if (!boost::regex_match(value.begin(), value.end(), match, regex)) {
    throw std::runtime_error("Invalid pagesize option");
  }
  char c = '\0';
  if (match.length(2) != 0) {
    c = char(tolower(value[size_t(match.position(2))]));
  }
  StringPiece numStr(value.data() + match.position(1), size_t(match.length(1)));
  size_t size = to<size_t>(numStr);
  switch (c) {
  case 't': size *= 1024;
  case 'g': size *= 1024;
  case 'm': size *= 1024;
  case 'k': size *= 1024;
  }
  return size;
}

/**
 * Get list of supported huge page sizes and their mount points, if
 * hugetlbfs file systems are mounted for those sizes.
 */
HugePageSizeVec readHugePageSizes() {
  HugePageSizeVec sizeVec = readRawHugePageSizes();
  if (sizeVec.empty()) {
    return sizeVec;  // nothing to do
  }
  std::sort(sizeVec.begin(), sizeVec.end());

  size_t defaultHugePageSize = getDefaultHugePageSize();

  struct PageSizeLess {
    bool operator()(const HugePageSize& a, size_t b) const {
      return a.size < b;
    }
    bool operator()(size_t a, const HugePageSize& b) const {
      return a < b.size;
    }
  };

  // Read and parse /proc/mounts
  std::vector<StringPiece> parts;
  std::vector<StringPiece> options;

  gen::byLine("/proc/mounts") | gen::eachAs<StringPiece>() |
    [&](StringPiece line) {
      parts.clear();
      split(" ", line, parts);
      // device path fstype options uid gid
      if (parts.size() != 6) {
        throw std::runtime_error("Invalid /proc/mounts line");
      }
      if (parts[2] != "hugetlbfs") {
        return;  // we only care about hugetlbfs
      }

      options.clear();
      split(",", parts[3], options);
      size_t pageSize = defaultHugePageSize;
      // Search for the "pagesize" option, which must have a value
      for (auto& option : options) {
        // key=value
        const char* p = static_cast<const char*>(
            memchr(option.data(), '=', option.size()));
        if (!p) {
          continue;
        }
        if (StringPiece(option.data(), p) != "pagesize") {
          continue;
        }
        pageSize = parsePageSizeValue(StringPiece(p + 1, option.end()));
        break;
      }

      auto pos = std::lower_bound(sizeVec.begin(), sizeVec.end(), pageSize,
                                  PageSizeLess());
      if (pos == sizeVec.end() || pos->size != pageSize) {
        throw std::runtime_error("Mount page size not found");
      }
      if (!pos->mountPoint.empty()) {
        // Only one mount point per page size is allowed
        return;
      }

      // Store mount point
      Path path(parts[1]);
      struct stat st;
      const int ret = stat(path.c_str(), &st);
      if (ret == -1 && errno == ENOENT) {
        return;
      }
      checkUnixError(ret, "stat hugepage mountpoint failed");
      pos->mountPoint = canonical(path);
      pos->device = st.st_dev;
    };

  return sizeVec;
}

} // namespace

const HugePageSizeVec& getHugePageSizes() {
  static HugePageSizeVec sizes = readHugePageSizes();
  return sizes;
}

const HugePageSize* getHugePageSize(size_t size) {
  // Linear search is just fine.
  for (auto& p : getHugePageSizes()) {
    if (p.mountPoint.empty()) {
      continue;
    }
    if (size == 0 || size == p.size) {
      return &p;
    }
  }
  return nullptr;
}

const HugePageSize* getHugePageSizeForDevice(dev_t device) {
  // Linear search is just fine.
  for (auto& p : getHugePageSizes()) {
    if (p.mountPoint.empty()) {
      continue;
    }
    if (device == p.device) {
      return &p;
    }
  }
  return nullptr;
}

} // namespace rdd
