/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>

namespace rdd {

inline int getCpuNum() {
  return sysconf(_SC_NPROCESSORS_CONF);
}

bool setCpuAffinity(int cpu, pid_t pid = 0);
int getCpuAffinity(pid_t pid = 0);

struct StatVFS {
  struct statvfs stats;

  StatVFS(const char* path) {
    statvfs(path, &stats);
  }

  size_t blockFree() const {
    return stats.f_bsize * stats.f_bfree;
  }
  size_t blockAvailable() const {
    return stats.f_bsize * stats.f_bavail;
  }
  size_t inodeFree() const {
    return stats.f_ffree;
  }
  size_t inodeAvailable() const {
    return stats.f_favail;
  }
};

struct SysInfo {
  struct sysinfo info;

  SysInfo() {
    sysinfo(&info);
  }

  size_t ramTotal() const {
    return info.totalram;
  }
  size_t ramFree() const {
    return info.freeram;
  }
};

struct ProcessInfo {
  size_t memTotal{0}; // memory VM size
  size_t memRSS{0};   // memory VM RSS

  ProcessInfo() {
    initMemory();
  }

 private:
  void initMemory();
};

} // namespace rdd
