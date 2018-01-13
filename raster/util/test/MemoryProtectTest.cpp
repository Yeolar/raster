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
