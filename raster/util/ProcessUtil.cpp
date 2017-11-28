/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/FileUtil.h"
#include "raster/util/Logging.h"
#include "raster/util/ProcessUtil.h"

namespace rdd {

pid_t ProcessUtil::readPid(const Path& file) {
  std::string s;
  readFile(file.c_str(), s);
  return to<pid_t>(s);
}

bool ProcessUtil::writePid(const Path& file, pid_t pid) {
  RDDCHECK(!file.exists()) << file << " already exist";
  return writeFile(to<std::string>(pid), file.c_str(), 0600);
}

} // namespace rdd
