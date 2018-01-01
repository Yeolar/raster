/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

// Basic usage of this class is very simple:
//
// In your .h file:
// class MyExpensiveService { ... };
//
// In your .cpp file (optional):
// namespace { rdd::Singleton<MyExpensiveService> the_singleton; }
//
// Code can access it via:
// MyExpensiveService* instance = Singleton<MyExpensiveService>::get();
//
// You also can directly access it by the variable defining the
// singleton rather than via get().
//
// You can have multiple singletons of the same underlying type, but
// each must be given a unique tag. If no tag is specified - default tag is used
//
// namespace {
// struct Tag1 {};
// struct Tag2 {};
// rdd::Singleton<MyExpensiveService> s_default;
// rdd::Singleton<MyExpensiveService, Tag1> s1;
// rdd::Singleton<MyExpensiveService, Tag2> s2;
// }
// ...
// MyExpensiveService* svc_default = s_default.get();
// MyExpensiveService* svc1 = s1.get();
// MyExpensiveService* svc2 = s2.get();
//
// By default, the singleton instance is constructed via new and
// deleted via delete, but this is configurable:
//
// namespace {
// rdd::Singleton<MyExpensiveService> the_singleton(create, destroy);
// }
//
// Where create and destroy are functions, Singleton<T>::CreateFunc
// Singleton<T>::TeardownFunc.
//
// For example, if you need to pass arguments to your class's constructor:
//   class X {
//    public:
//      X(int a1, std::string a2);
//    // ...
//   }
// Make your singleton like this:
//   rdd::Singleton<X> singleton_x([]() { return new X(42, "foo"); });
//

#pragma once

#include <atomic>
#include <memory>
#include <mutex>

#include "raster/util/Macro.h"

namespace rdd {

namespace detail {

struct DefaultTag {};

// An actual instance of a singleton, tracking the instance itself
template <typename T>
struct SingletonHolder {
 public:
  typedef std::function<void(T*)> TeardownFunc;
  typedef std::function<T*(void)> CreateFunc;

  template <typename Tag>
  inline static SingletonHolder<T>& singleton() {
    static auto entry = new SingletonHolder<T>();
    return *entry;
  }

  inline T* get() {
    if (LIKELY(state_.load(std::memory_order_acquire) == LIVING)) {
      return instance_ptr_;
    }
    createInstance();
    return instance_ptr_;
  }

  void registerSingleton(CreateFunc c, TeardownFunc t) {
    std::lock_guard<std::mutex> guard(lock_);
    create_ = std::move(c);
    teardown_ = std::move(t);
    state_ = DEAD;
  }

  SingletonHolder(const SingletonHolder&) = delete;
  SingletonHolder& operator=(const SingletonHolder&) = delete;

 private:
  enum {
    UNREGISTERED,
    DEAD,
    LIVING,
  };

  SingletonHolder() {}

  void createInstance() {
    std::lock_guard<std::mutex> guard(lock_);
    if (state_.load(std::memory_order_acquire) == LIVING) {
      return;
    }
    std::shared_ptr<T> instance(
        create_ ? create_() : new T,
        teardown_ ? teardown_ : [](T* v) { delete v; });

    instance_ptr_ = instance.get();
    instance_ = std::move(instance);
    state_.store(LIVING, std::memory_order_release);
  }

  std::mutex lock_;
  std::atomic<int> state_{UNREGISTERED};
  std::shared_ptr<T> instance_;
  T* instance_ptr_{nullptr};
  CreateFunc create_{nullptr};
  TeardownFunc teardown_{nullptr};
};

} // namespace detail

template <typename T, typename Tag = detail::DefaultTag>
class Singleton {
 public:
  typedef std::function<T*(void)> CreateFunc;
  typedef std::function<void(T*)> TeardownFunc;

  static T* get() { return getEntry().get(); }

  explicit Singleton(typename Singleton::CreateFunc c = nullptr,
                     typename Singleton::TeardownFunc t = nullptr) {
    getEntry().registerSingleton(c, t);
  }

 private:
  inline static detail::SingletonHolder<T>& getEntry() {
    return detail::SingletonHolder<T>::template singleton<Tag>();
  }
};

} // namespace rdd
