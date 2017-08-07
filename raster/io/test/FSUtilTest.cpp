/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/FSUtil.h"
#include "raster/util/Logging.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(fs, directory) {
  fs::path dir = fs::current_path();
  if (fs::is_directory(dir)) {
    for (fs::directory_iterator it(dir); it != fs::directory_iterator(); ++it) {
      RDDLOG(INFO) << it->path();
      RDDLOG(INFO) << it->path().filename();
    }
  }
}
