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
  ~ReflectInfoT() override {}

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

// need wraps with namespace rdd {} when using
#define RDD_RF_REG(r, name) \
  template <>               \
  ReflectInfoT<r, name> ReflectObject<r, name>::ri_(#name)

} // namespace rdd
