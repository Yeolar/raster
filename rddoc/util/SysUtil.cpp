/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/SysUtil.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>

namespace rdd {

bool setCpuAffinity(int cpu, pid_t pid) {
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu, &mask);
  return sched_setaffinity(pid, sizeof(mask), &mask) == 0;
}

int getCpuAffinity(pid_t pid) {
  cpu_set_t mask;
  CPU_ZERO(&mask);
  if (sched_setaffinity(pid, sizeof(mask), &mask) == 0) {
    int n = getCpuNum();
    for (int i = 0; i < n; ++i) {
      if (CPU_ISSET(i, &mask)) {
        return i;
      }
    }
  }
  return -1;
}

static unsigned long extractNumeric(char* p) {
  p[strlen(p) - 3] = '\0';
  while (*p < '0' || *p > '9') p++;
  return atol(p) * 1024;
}

void Resource::initMemory() {
  struct sysinfo si;
  sysinfo(&si);
  mem_t = si.totalram;
  mem_f = si.freeram;
}

void Resource::initProcMemory() {
  FILE* file = fopen("/proc/self/status", "r");
  char line[128];
  while (fgets(line, 128, file) != nullptr) {
    if (strncmp(line, "VmRSS:", 6) == 0) {
      mem_proc_r = extractNumeric(line);
    }
    if (strncmp(line, "VmSize:", 7) == 0) {
      mem_proc_t = extractNumeric(line);
    }
  }
  fclose(file);
}

}
