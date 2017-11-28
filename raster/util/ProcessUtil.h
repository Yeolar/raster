/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/Path.h"

namespace rdd {

class ProcessUtil {
public:
  static pid_t readPid(const Path& file);

  static bool writePid(const Path& file, pid_t pid);
};

} // namespace rdd
