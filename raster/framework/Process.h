/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/FileUtil.h"

namespace rdd {

class ProcessUtil {
public:
  static pid_t readPid(const char* file) {
    std::string s;
    readFile(file, s);
    return to<pid_t>(s);
  }

  static bool writePid(const char* file, pid_t pid) {
    return writeFile(to<std::string>(pid), file, 0600);
  }
};

} // namespace rdd
