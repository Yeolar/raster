/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <sys/stat.h>
#include <sys/types.h>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <boost/operators.hpp>
#include "raster/io/FSUtil.h"

namespace rdd {

struct HugePageSize : private boost::totally_ordered<HugePageSize> {
  explicit HugePageSize(size_t s) : size(s) { }

  fs::path filePath(const fs::path& relpath) const {
    return mountPoint / relpath;
  }

  size_t size = 0;
  fs::path mountPoint;
  dev_t device = 0;
};

inline bool operator<(const HugePageSize& a, const HugePageSize& b) {
  return a.size < b.size;
}

inline bool operator==(const HugePageSize& a, const HugePageSize& b) {
  return a.size == b.size;
}

/**
 * Vector of (huge_page_size, mount_point), sorted by huge_page_size.
 * mount_point might be empty if no hugetlbfs file system is mounted for
 * that size.
 */
typedef std::vector<HugePageSize> HugePageSizeVec;

/**
 * Get list of supported huge page sizes and their mount points, if
 * hugetlbfs file systems are mounted for those sizes.
 */
const HugePageSizeVec& getHugePageSizes();

/**
 * Return the mount point for the requested huge page size.
 * 0 = use smallest available.
 * Returns nullptr if the requested huge page size is not available.
 */
const HugePageSize* getHugePageSize(size_t size = 0);

/**
 * Return the huge page size for a device.
 * returns nullptr if device does not refer to a huge page filesystem.
 */
const HugePageSize* getHugePageSizeForDevice(dev_t device);

} // namespace rdd
