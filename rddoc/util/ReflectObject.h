/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include "rddoc/util/Singleton.h"

namespace rdd {

template <class R>
class ReflectInfo {
public:
  typedef std::map<std::string, ReflectInfo<R>*> ReflectInfoMap;

  ReflectInfo() {}
  virtual ~ReflectInfo() {}

  virtual R* create() const = 0;

  static ReflectInfoMap* get() {
    return Singleton<ReflectInfoMap>::get();
  }
};

template <class R, class T>
class ReflectInfoT : public ReflectInfo<R> {
public:
  ReflectInfoT(const std::string& name) : name_(name) {
    registerReflectObject();
  }
  virtual ~ReflectInfoT() {}

  virtual R* create() const { return new T(); }
  std::string name() const { return name_; }

  void registerReflectObject() {
    auto* p = ReflectInfo<R>::get();
    p->emplace(name(), this);
  }

private:
  std::string name_;
};

template <class R, class T>
class ReflectObject : public R {
public:
  virtual ~ReflectObject() {}

private:
  static ReflectInfoT<R, T> ri_;
};

template <class R>
R* makeReflectObject(const std::string& name) {
  auto* p = ReflectInfo<R>::get();
  auto it = p->find(name);
  return it != p->end() ? it->second->create() : nullptr;
}

template <class R>
std::shared_ptr<R> makeSharedReflectObject(const std::string& name) {
  return std::shared_ptr<R>(makeReflectObject<R>(name));
}

#define RDD_RF_REG(r, name) \
  template <>               \
  rdd::ReflectInfoT<r, name> rdd::ReflectObject<r, name>::ri_(#name)

} // namespace rdd
