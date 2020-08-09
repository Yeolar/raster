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

#pragma once

#include <vector>
#include <execinfo.h>
#include <unistd.h>
#include <string>


namespace acc {

inline std::vector<std::string> recordBacktrace() {
  std::vector<std::string> out;
  void* array[128];
  int n = backtrace(array, 128);
  char** lines = backtrace_symbols(array, n);
  if (lines != nullptr && n > 0) {
    for (int i = 1; i < n; i++) {
      out.emplace_back(lines[i]);
    }
  }
  free(lines);
  return out;
}

} // namespace acc
