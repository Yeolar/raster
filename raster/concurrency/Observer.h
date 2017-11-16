/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>

#include "raster/util/Memory.h"

namespace rdd {

template <class T> struct FunctionObserver;

template <class T>
struct Observer {
  virtual void on(const T&) = 0;

  virtual ~Observer() {}

  template <class N>
  static std::unique_ptr<Observer> create(N&& onFn) {
    return rdd::make_unique<FunctionObserver<T>>(std::forward<N>(onFn));
  }
};

template <class T>
struct FunctionObserver : public Observer<T> {
  typedef std::function<void(const T&)> On;

  template <class N = On>
  FunctionObserver(N&& n)
    : on_(std::forward<N>(n)) {}

  void on(const T& val) override {
    if (on_) on_(val);
  }

 protected:
  On on_;
};

template <class T> using ObserverPtr = std::shared_ptr<Observer<T>>;

} // namespace rdd
