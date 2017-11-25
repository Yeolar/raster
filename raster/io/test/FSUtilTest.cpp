/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/FSUtil.h"
#include "raster/util/Logging.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(fs, directory) {
  Path dir = currentPath();
  if (dir.isDirectory()) {
    for (auto& file : ls(dir)) {
      RDDLOG(INFO) << dir / file;
      RDDLOG(INFO) << file;
    }
  }
}
