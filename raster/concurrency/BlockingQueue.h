/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <queue>
#include <boost/thread/shared_mutex.hpp>

#include <accelerator/thread/Semaphore.h>
#include <accelerator/thread/Synchronized.h>

#include "raster/concurrency/MPMCQueue.h"

namespace raster {

template <class T>
class BlockingQueue {
 public:
  virtual ~BlockingQueue() {}

  virtual void add(T item) = 0;
  virtual T take() = 0;
  virtual size_t size() = 0;
};

template <class T>
class GenericBlockingQueue : public BlockingQueue<T> {
 public:
  GenericBlockingQueue() {}

  void add(T item) override {
    queue_.wlock()->push(std::move(item));
    sem_.post();
  }

  T take() override {
    while (true) {
      {
        auto ulockedQueue = queue_.ulock();
        if (!ulockedQueue->empty()) {
          auto wlockedQueue = ulockedQueue.moveFromUpgradeToWrite();
          T item = std::move(wlockedQueue->front());
          wlockedQueue->pop();
          return item;
        }
      }
      sem_.wait();
    }
  }

  size_t size() override {
    return queue_.rlock()->size();
  }

 private:
  acc::Semaphore sem_;
  acc::Synchronized<std::queue<T>, boost::shared_mutex> queue_;
};

template <class T>
class MPMCBlockingQueue : public BlockingQueue<T> {
 public:
  // Note: The queue pre-allocates all memory for max_capacity
  explicit MPMCBlockingQueue(size_t max_capacity)
      : queue_(max_capacity) {}

  void add(T item) override {
    if (!queue_.write(std::move(item))) {
      throw std::runtime_error("MPMCQueue full, can't add item");
    }
    sem_.post();
  }

  T take() override {
    T item;
    while (!queue_.readIfNotEmpty(item)) {
      sem_.wait();
    }
    return item;
  }

  size_t capacity() {
    return queue_.capacity();
  }

  size_t size() override {
    return queue_.size();
  }

 private:
  acc::Semaphore sem_;
  MPMCQueue<T> queue_;
};

} // namespace raster
