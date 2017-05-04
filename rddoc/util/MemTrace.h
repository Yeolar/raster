/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdlib.h>
#include <new>

//#define RDD_DEBUG_NEW_NO_NEW_REDEFINITION

namespace rdd {

bool checkLeaks();

}

void* operator new(size_t size, const char* file, int line);
void* operator new[](size_t size, const char* file, int line);
void operator delete(void* pointer, const char* file, int line);
void operator delete[](void* pointer, const char* file, int line);

#ifndef RDD_DEBUG_NEW_NO_NEW_REDEFINITION
#define new RDD_DEBUG_NEW
#define RDD_DEBUG_NEW new(__FILE__, __LINE__)
#define rdd_debug_new new
#else
#define rdd_debug_new new(__FILE__, __LINE__)
#endif

#ifdef RDD_DEBUG_NEW_EMULATE_MALLOC
#include <stdlib.h>
#define malloc(s) ((void*)(rdd_debug_new char[s]))
#define free(p) delete[] (char*)(p)
#endif

namespace rdd {

extern bool new_verbose_flag;   // default to false: no verbose information

}

