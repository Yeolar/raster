/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/MemoryProtect.h"
#include "raster/framework/Signal.h"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

using namespace rdd;

int main(int argc, char *argv[]) {
  setupMemoryProtectSignal();

  int pagesize = sysconf(_SC_PAGESIZE);

  /* Allocate a buffer aligned on a page boundary;
   * initial protection is PROT_READ | PROT_WRITE */

  char* buffer = (char*) memalign(pagesize, pagesize * 4);
  if (buffer == nullptr) {
    perror("memalign");
    exit(1);
  }

  char* ban = buffer + pagesize * 2;
  char* end = buffer + pagesize * 4;

  printf("Start of region: %p, pagesize: %d\n", buffer, pagesize);
  printf("Ban memory: %p\n\n", ban);

  MemoryProtect(ban).ban();

  for (char* p = buffer; p < end; ) {
    *(p++) = 'a';
  }
  printf("Loop completed\n");
}
