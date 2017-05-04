/*
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

#pragma once

#include <atomic>
#include <memory>
#include "rddoc/util/Lock.h"
#include "rddoc/util/Macro.h"

namespace rdd {

namespace detail {

struct DefaultTag {};

// An actual instance of a singleton, tracking the instance itself
template <typename T>
struct SingletonHolder : noncopyable {
public:
  template <typename Tag>
  inline static SingletonHolder<T>& singleton() {
    static auto entry = new SingletonHolder<T>();
    return *entry;
  }

  inline T* get() {
    if (LIKELY(state_ == LIVING)) {
      return instance_ptr_;
    }
    createInstance();
    return instance_ptr_;
  }

private:
  enum {
    UNREGISTERED,
    LIVING,
  };

  SingletonHolder() : state_(UNREGISTERED) {}

  void createInstance() {
    LockGuard guard(lock_);
    instance_ = std::make_shared<T>();
    instance_ptr_ = instance_.get();
    state_.store(LIVING);
  }

  Lock lock_;
  std::atomic<int> state_;
  std::shared_ptr<T> instance_;
  T* instance_ptr_{nullptr};
};

}

template <typename T, typename Tag = detail::DefaultTag>
class Singleton {
public:
  static T* get() { return getEntry().get(); }

  Singleton() {}

private:
  inline static detail::SingletonHolder<T>& getEntry() {
    return detail::SingletonHolder<T>::template singleton<Tag>();
  }
};

}
