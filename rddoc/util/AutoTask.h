/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <mutex>
#include <thread>
#include <stdint.h>
#include <unistd.h>
#include "rddoc/util/ThreadUtil.h"
#include "rddoc/util/Time.h"

namespace rdd {

class AutoTask {
public:
  virtual void update() = 0;

  bool checkStamp(uint64_t stamp) {
    return stamp >= stamp_ ? (stamp_ = stamp + 1) : false;
  }

private:
  uint64_t stamp_{0};
};

class AutoTaskManager {
public:
  AutoTaskManager()
    : handle_(std::thread(&AutoTaskManager::run, this)) {
    setThreadName(handle_.native_handle(), "AutoTaskThread");
  }

  virtual ~AutoTaskManager() {
    handle_.join();
  }

  void run() {
    initStamp();
    while (true) {
      runTasks();
      usleep(100000); // 100ms
    }
  }

  void initStamp() { timestamp_ = timestampNow(); }

  template <class T>
  void addTask(T* task, uint64_t interval) {
    if (task != nullptr) {
      std::lock_guard<std::mutex> guard(lock_);
      tasks_.emplace((AutoTask*)task, interval);
    }
  }

  void runTasks() {
    uint64_t passed = timePassed(timestamp_);
    std::map<AutoTask*, uint64_t> tasks;
    {
      std::lock_guard<std::mutex> guard(lock_);
      tasks = tasks_;
    }
    for (auto& kv : tasks) {
      if (kv.first->checkStamp(passed / kv.second)) {
        kv.first->update();
      }
    }
  }

private:
  uint64_t timestamp_{0};
  std::map<AutoTask*, uint64_t> tasks_;
  std::mutex lock_;
  std::thread handle_;
};

}

