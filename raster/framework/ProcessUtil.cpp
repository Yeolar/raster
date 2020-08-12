/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/framework/ProcessUtil.h"

#include <accelerator/FileUtil.h>
#include <accelerator/Logging.h>

namespace acc {

pid_t readPid(const Path& file) {
  std::string s;
  readFile(file.c_str(), s);
  return to<pid_t>(s);
}

bool writePid(const Path& file, pid_t pid) {
  ACCCHECK(!file.exists()) << file << " already exist";
  return writeFile(to<std::string>(pid), file.c_str(), 0600);
}

} // namespace acc
