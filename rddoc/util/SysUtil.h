/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>

namespace rdd {

inline void asm_volatile_pause() {
  asm volatile ("pause");
}

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

  size_t blockFree() const { return stats.f_bsize * stats.f_bfree; }
  size_t blockAvailable() const { return stats.f_bsize * stats.f_bavail; }
  size_t inodeFree() const { return stats.f_ffree; }
  size_t inodeAvailable() const { return stats.f_favail; }
};

struct SysInfo {
  struct sysinfo info;

  SysInfo() {
    sysinfo(&info);
  }

  size_t ramTotal() const { return info.totalram; }
  size_t ramFree() const { return info.freeram; }
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

}
