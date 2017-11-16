/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <assert.h>
#include <atomic>

#include "raster/util/SpinLock.h"

namespace rdd {

/*
 * Usage:
 *
 * class MyClass {
 *   LinkedListHook<MyClass> hook_;
 * }
 *
 * LinkedList<MyClass, &MyClass::hook_> list;
 * list.pushHead(&a);
 * list.sweep([] (MyClass* c) { doSomething(c); }
 */

template <class T>
struct LinkedListHook {
  T* next{nullptr};
};

template <class T, LinkedListHook<T> T::*HookMember>
class LinkedList {
public:
  LinkedList() {}
  ~LinkedList() {
    assert(empty());
  }

  bool empty() const { return head_ == nullptr; }
  size_t size() const { return count_; }

  void pushHead(T* t) {
    assert(next(t) == nullptr);
    SpinLockGuard guard(lock_);
    next(t) = head_;
    head_ = t;
    ++count_;
  }

  T* popHead() {
    SpinLockGuard guard(lock_);
    if (empty()) {
      return nullptr;
    }
    auto t = head_;
    head_ = next(t);
    next(t) = nullptr;
    --count_;
    return t;
  }

  template <typename F>
  void sweep(F&& func) {
    T* rhead = nullptr;
    {
      SpinLockGuard guard(lock_);
      rhead = reverse(head_); // sweep on inserted order
      head_ = nullptr;
      count_ = 0;
    }
    while (rhead != nullptr) {
      auto t = rhead;
      rhead = next(t);
      next(t) = nullptr;
      func(t);
    }
  }

  NOCOPY(LinkedList);

private:
  T* head_{nullptr};
  SpinLock lock_;
  std::atomic<size_t> count_{0};

  static T*& next(T* t) { return (t->*HookMember).next; }

  static T* reverse(T* head) {
    T* rhead = nullptr;
    while (head != nullptr) {
      auto t = head;
      head = next(t);
      next(t) = rhead;
      rhead = t;
    }
    return rhead;
  }
};

} // namespace rdd
