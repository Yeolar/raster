/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <atomic>
#include <cassert>
#include <utility>

#include "raster/util/Memory.h"

namespace rdd {

/**
 * A very simple atomic single-linked list primitive.
 *
 * Usage:
 *
 * class MyClass {
 *   AtomicIntrusiveLinkedListHook<MyClass> hook_;
 * }
 *
 * AtomicIntrusiveLinkedList<MyClass, &MyClass::hook_> list;
 * list.insert(&a);
 * list.sweep([] (MyClass* c) { doSomething(c); }
 */
template <class T>
struct AtomicIntrusiveLinkedListHook {
  T* next{nullptr};
};

template <class T, AtomicIntrusiveLinkedListHook<T> T::*HookMember>
class AtomicIntrusiveLinkedList {
 public:
  AtomicIntrusiveLinkedList() {}
  AtomicIntrusiveLinkedList(const AtomicIntrusiveLinkedList&) = delete;
  AtomicIntrusiveLinkedList& operator=(const AtomicIntrusiveLinkedList&) =
      delete;
  AtomicIntrusiveLinkedList(AtomicIntrusiveLinkedList&& other) noexcept {
    auto tmp = other.head_.load();
    other.head_ = head_.load();
    head_ = tmp;
  }
  AtomicIntrusiveLinkedList& operator=(
      AtomicIntrusiveLinkedList&& other) noexcept {
    auto tmp = other.head_.load();
    other.head_ = head_.load();
    head_ = tmp;

    return *this;
  }

  /**
   * Note: list must be empty on destruction.
   */
  ~AtomicIntrusiveLinkedList() {
    assert(empty());
  }

  bool empty() const {
    return head_.load() == nullptr;
  }

  /**
   * Atomically insert t at the head of the list.
   * @return True if the inserted element is the only one in the list
   *         after the call.
   */
  bool insertHead(T* t) {
    assert(next(t) == nullptr);

    auto oldHead = head_.load(std::memory_order_relaxed);
    do {
      next(t) = oldHead;
      /* oldHead is updated by the call below.

         NOTE: we don't use next(t) instead of oldHead directly due to
         compiler bugs (GCC prior to 4.8.3 (bug 60272), clang (bug 18899),
         MSVC (bug 819819); source:
         http://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange */
    } while (!head_.compare_exchange_weak(oldHead, t,
                                          std::memory_order_release,
                                          std::memory_order_relaxed));

    return oldHead == nullptr;
  }

  /**
   * Repeatedly replaces the head with nullptr,
   * and calls func() on the removed elements in the order from tail to head.
   * Stops when the list is empty.
   */
  template <typename F>
  void sweep(F&& func) {
    while (auto head = head_.exchange(nullptr)) {
      auto rhead = reverse(head);
      unlinkAll(rhead, std::forward<F>(func));
    }
  }

  /**
   * Similar to sweep() but calls func() on elements in LIFO order.
   *
   * func() is called for all elements in the list at the moment
   * reverseSweep() is called.  Unlike sweep() it does not loop to ensure the
   * list is empty at some point after the last invocation.  This way callers
   * can reason about the ordering: elements inserted since the last call to
   * reverseSweep() will be provided in LIFO order.
   *
   * Example: if elements are inserted in the order 1-2-3, the callback is
   * invoked 3-2-1.  If the callback moves elements onto a stack, popping off
   * the stack will produce the original insertion order 1-2-3.
   */
  template <typename F>
  void reverseSweep(F&& func) {
    // We don't loop like sweep() does because the overall order of callbacks
    // would be strand-wise LIFO which is meaningless to callers.
    auto head = head_.exchange(nullptr);
    unlinkAll(head, std::forward<F>(func));
  }

 private:
  std::atomic<T*> head_{nullptr};

  static T*& next(T* t) {
    return (t->*HookMember).next;
  }

  /* Reverses a linked list, returning the pointer to the new head
     (old tail) */
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

  /* Unlinks all elements in the linked list fragment pointed to by `head',
   * calling func() on every element */
  template <typename F>
  void unlinkAll(T* head, F&& func) {
    while (head != nullptr) {
      auto t = head;
      head = next(t);
      next(t) = nullptr;
      func(t);
    }
  }
};

/**
 * A very simple atomic single-linked list primitive.
 *
 * Usage:
 *
 * AtomicLinkedList<MyClass> list;
 * list.insert(a);
 * list.sweep([] (MyClass& c) { doSomething(c); }
 */

template <class T>
class AtomicLinkedList {
 public:
  AtomicLinkedList() {}
  AtomicLinkedList(const AtomicLinkedList&) = delete;
  AtomicLinkedList& operator=(const AtomicLinkedList&) = delete;
  AtomicLinkedList(AtomicLinkedList&& other) noexcept = default;
  AtomicLinkedList& operator=(AtomicLinkedList&& other) = default;

  ~AtomicLinkedList() {
    sweep([](T&&) {});
  }

  bool empty() const {
    return list_.empty();
  }

  /**
   * Atomically insert t at the head of the list.
   * @return True if the inserted element is the only one in the list
   *         after the call.
   */
  bool insertHead(T t) {
    auto wrapper = make_unique<Wrapper>(std::move(t));

    return list_.insertHead(wrapper.release());
  }

  /**
   * Repeatedly pops element from head,
   * and calls func() on the removed elements in the order from tail to head.
   * Stops when the list is empty.
   */
  template <typename F>
  void sweep(F&& func) {
    list_.sweep([&](Wrapper* wrapperPtr) mutable {
      std::unique_ptr<Wrapper> wrapper(wrapperPtr);

      func(std::move(wrapper->data));
    });
  }

  /**
   * Similar to sweep() but calls func() on elements in LIFO order.
   *
   * func() is called for all elements in the list at the moment
   * reverseSweep() is called.  Unlike sweep() it does not loop to ensure the
   * list is empty at some point after the last invocation.  This way callers
   * can reason about the ordering: elements inserted since the last call to
   * reverseSweep() will be provided in LIFO order.
   *
   * Example: if elements are inserted in the order 1-2-3, the callback is
   * invoked 3-2-1.  If the callback moves elements onto a stack, popping off
   * the stack will produce the original insertion order 1-2-3.
   */
  template <typename F>
  void reverseSweep(F&& func) {
    list_.reverseSweep([&](Wrapper* wrapperPtr) mutable {
      std::unique_ptr<Wrapper> wrapper(wrapperPtr);

      func(std::move(wrapper->data));
    });
  }

 private:
  struct Wrapper {
    explicit Wrapper(T&& t) : data(std::move(t)) {}

    AtomicIntrusiveLinkedListHook<Wrapper> hook;
    T data;
  };
  AtomicIntrusiveLinkedList<Wrapper, &Wrapper::hook> list_;
};

} // namespace rdd
