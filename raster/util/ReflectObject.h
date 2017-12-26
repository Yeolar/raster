/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <memory>
#include <string>

#include "raster/util/Singleton.h"

namespace rdd {

template <class R>
class ReflectInfo {
 public:
  typedef std::map<const char*, ReflectInfo<R>*> ReflectInfoMap;

  virtual ~ReflectInfo() {}

  virtual R* create() const = 0;

  static ReflectInfoMap* get() {
    return Singleton<ReflectInfoMap>::get();
  }
};

template <class R, class T>
class ReflectInfoT : public ReflectInfo<R> {
 public:
  ReflectInfoT(const char* name) : name_(name) {
    registerReflectObject();
  }
  virtual ~ReflectInfoT() {}

  R* create() const override { return new T(); }

 private:
  void registerReflectObject() {
    ReflectInfo<R>::get()->emplace(name_, this);
  }

  const char* name_;
};

template <class R, class T>
class ReflectObject : public R {
 public:
  virtual ~ReflectObject() {}

 private:
  static ReflectInfoT<R, T> ri_;
};

template <class R>
R* makeReflectObject(const char* name) {
  auto* p = ReflectInfo<R>::get();
  auto it = p->find(name);
  return it != p->end() ? it->second->create() : nullptr;
}

template <class R>
std::shared_ptr<R> makeSharedReflectObject(const char* name) {
  return std::shared_ptr<R>(makeReflectObject<R>(name));
}

template <class R>
std::unique_ptr<R> makeUniqueReflectObject(const char* name) {
  return std::unique_ptr<R>(makeReflectObject<R>(name));
}

#define RDD_RF_REG(r, name) \
  template <>               \
  ::rdd::ReflectInfoT<r, name> ::rdd::ReflectObject<r, name>::ri_(#name)

} // namespace rdd
