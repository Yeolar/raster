/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <string>
#include <unistd.h>
#include <sys/sysinfo.h>

namespace rdd {

inline int getCpuNum() {
  return sysconf(_SC_NPROCESSORS_CONF);
}

bool setCpuAffinity(int cpu, pid_t pid = 0);
int getCpuAffinity(pid_t pid = 0);

struct Resource {
  unsigned long mem_t;        // total memory
  unsigned long mem_f;        // free memory
  unsigned long mem_proc_t;   // process memory VM size
  unsigned long mem_proc_r;   // process memory VM RSS

  Resource()
    : mem_t(0)
    , mem_f(0)
    , mem_proc_t(0)
    , mem_proc_r(0) {
    initMemory();
    initProcMemory();
  }

private:
  void initMemory();
  void initProcMemory();
};

}
