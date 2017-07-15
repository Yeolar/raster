/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <unistd.h>
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

struct Resource {
  unsigned long tMemory;      // total memory
  unsigned long fMemory;      // free memory
  unsigned long tProcMemory;  // process memory VM size
  unsigned long rProcMemory;  // process memory VM RSS

  Resource()
    : tMemory(0)
    , fMemory(0)
    , tProcMemory(0)
    , rProcMemory(0) {
    initMemory();
    initProcMemory();
  }

private:
  void initMemory();
  void initProcMemory();
};

}
