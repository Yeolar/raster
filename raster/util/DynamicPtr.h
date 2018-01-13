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

#include <cstdlib>
#include <utility>

namespace rdd {

class DeleterBase {
 public:
  virtual ~DeleterBase() {}
  virtual void dispose(void* pointer) const = 0;
};

template <class T>
class SimpleDeleter : public DeleterBase {
 public:
  void dispose(void* pointer) const override {
    delete static_cast<T*>(pointer);
  }
};

class DynamicPtr {
 public:
  DynamicPtr() : pointer_(nullptr), deleter_(nullptr) {}

  ~DynamicPtr() {
    dispose();
  }

  template <class T, class Deleter = SimpleDeleter<T>>
  void set(T* pointer) {
    dispose();
    if (pointer) {
      pointer_ = pointer;
      deleter_ = new Deleter();
    }
  }

  template <class T>
  T* get() const {
    return static_cast<T*>(pointer_);
  }

  explicit operator bool() const {
    return pointer_ != nullptr;
  }

  void* release() {
    auto pointer = pointer_;
    clean();
    return pointer;
  }

  bool dispose() {
    if (pointer_ != nullptr) {
      deleter_->dispose(pointer_);
      clean();
      return true;
    }
    return false;
  }

  void swap(DynamicPtr& other) {
    std::swap(pointer_, other.pointer_);
    std::swap(deleter_, other.deleter_);
  }

  DynamicPtr(const DynamicPtr&) = delete;
  DynamicPtr& operator=(const DynamicPtr&) = delete;

 private:

  void clean() {
    pointer_ = nullptr;
    if (deleter_) {
      delete deleter_;
      deleter_ = nullptr;
    }
  }

  void* pointer_;
  DeleterBase* deleter_;
};

} // namespace rdd
