/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/io/Path.h"

namespace rdd {

pid_t readPid(const Path& file);

bool writePid(const Path& file, pid_t pid);

} // namespace rdd
