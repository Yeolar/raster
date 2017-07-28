/*
 * Copyright (C) 2017, Yeolar
 */

#include <mutex>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef RDD_DEBUG_NEW_HASH_BSIZE
#define RDD_DEBUG_NEW_HASH_BSIZE 16384
#endif

#ifndef RDD_DEBUG_NEW_HASH
#define RDD_DEBUG_NEW_HASH(p) \
  (((unsigned long long)(p) >> 8) % RDD_DEBUG_NEW_HASH_BSIZE)
#endif

namespace rdd {

struct NewPtrNode {
  NewPtrNode* next;
  const char* file;
  int         line;
  size_t      size;
};

struct NewPtrList {
  NewPtrNode* head;
  std::mutex  lock;
};

static NewPtrList new_ptr_list[RDD_DEBUG_NEW_HASH_BSIZE];

bool new_verbose_flag = false;

bool checkLeaks() {
  bool leaked = false;
  time_t t = time(nullptr);
  char name[32];
  strftime(name, sizeof(name), "memtrace_%y%m%d%H%M%S", localtime(&t));
  FILE* fp = fopen(name, "w");
  for (int i = 0; i < RDD_DEBUG_NEW_HASH_BSIZE; ++i) {
    std::lock_guard<std::mutex> guard(new_ptr_list[i].lock);
    NewPtrNode* ptr = new_ptr_list[i].head;
    if (ptr) {
      leaked = true;
      while (ptr) {
        fprintf(fp, "trace: %s:%d object: %p(%zu)\n",
                ptr->file,
                ptr->line,
                (char*)ptr + sizeof(NewPtrNode),
                ptr->size);
        ptr = ptr->next;
      }
    }
  }
  fclose(fp);
  return leaked;
}

} // namespace rdd

void* operator new(size_t size, const char* file, int line) {
  size_t s = size + sizeof(rdd::NewPtrNode);
  rdd::NewPtrNode* ptr = (rdd::NewPtrNode*)malloc(s);
  if (ptr == nullptr) {
    fprintf(stderr, "new: out of memory when allocating %zu bytes\n", size);
    abort();
  }
  void* pointer = (char*)ptr + sizeof(rdd::NewPtrNode);
  size_t i = RDD_DEBUG_NEW_HASH(pointer);
  {
    std::lock_guard<std::mutex> guard(rdd::new_ptr_list[i].lock);
    ptr->next = rdd::new_ptr_list[i].head;
    ptr->file = file;
    ptr->line = line;
    ptr->size = size;
    rdd::new_ptr_list[i].head = ptr;
  }
  if (rdd::new_verbose_flag) {
    printf("new: allocated %p (size %zu, %s:%d)\n", pointer, size, file, line);
  }
  return pointer;
}

void* operator new[](size_t size, const char* file, int line) {
  return operator new(size, file, line);
}

void* operator new(size_t size) {
  return operator new(size, "<Unknown>", 0);
}

void* operator new[](size_t size) {
  return operator new(size);
}

void* operator new(size_t size, const std::nothrow_t&) throw() {
  return operator new(size);
}

void* operator new[](size_t size, const std::nothrow_t&) throw() {
  return operator new[](size);
}

void operator delete(void* pointer) {
  if (pointer == nullptr) {
    return;
  }
  size_t i = RDD_DEBUG_NEW_HASH(pointer);
  rdd::NewPtrNode* ptr = nullptr;
  rdd::NewPtrNode* ptr_last = nullptr;
  {
    std::lock_guard<std::mutex> guard(rdd::new_ptr_list[i].lock);
    ptr = rdd::new_ptr_list[i].head;
    while (ptr) {
      if ((char*)ptr + sizeof(rdd::NewPtrNode) != pointer) {
        ptr_last = ptr;
        ptr = ptr->next;
      } else {
        if (ptr_last) {
          ptr_last->next = ptr->next;
        } else {
          rdd::new_ptr_list[i].head = ptr->next;
        }
        break;
      }
    }
  }
  if (ptr) {
    if (rdd::new_verbose_flag) {
      printf("delete: freeing %p (size %zu)\n", pointer, ptr->size);
    }
    free(ptr);
    return;
  }
  fprintf(stderr, "delete: invalid pointer %p\n", pointer);
  abort();
}

void operator delete[](void* pointer) {
  operator delete(pointer);
}

void operator delete(void* pointer, const char* file, int line) {
  if (rdd::new_verbose_flag) {
    printf("info: exception thrown on initializing object at %p (%s:%d)\n",
           pointer, file, line);
  }
  operator delete(pointer);
}

void operator delete[](void* pointer, const char* file, int line) {
  operator delete(pointer, file, line);
}

void operator delete(void* pointer, const std::nothrow_t&) {
  operator delete(pointer, "<Unknown>", 0);
}

void operator delete[](void* pointer, const std::nothrow_t&) {
  operator delete(pointer, std::nothrow);
}

