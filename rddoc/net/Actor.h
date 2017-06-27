/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/concurrency/CPUThreadPool.h"
#include "rddoc/io/event/Event.h"
#include "rddoc/io/event/EventGroup.h"
#include "rddoc/io/event/EventLoop.h"
#include "rddoc/net/Channel.h"
#include "rddoc/net/Processor.h"
#include "rddoc/net/Service.h"
#include "rddoc/util/Function.h"
#include "rddoc/util/MapUtil.h"
#include "rddoc/util/Signal.h"
#include "rddoc/util/Singleton.h"
#include "rddoc/util/SysUtil.h"

namespace rdd {

class AsyncClient;

class Actor {
public:
  struct Options {
    size_t stackSize{256*1024};
    size_t connectionLimit{100000};
    size_t fiberLimit{4000};
    uint64_t pollSize{1024};
    uint64_t pollTimeout{1000};
    bool forwarding{false};
  };

  struct ForwardTarget {
    int port{0};
    Peer fpeer;
    int flow{0};
  };

  Actor() {}

  void setOptions(const Options& options) {
    options_ = options;
  }

  void start();

  void addService(const std::string& name,
                  const std::shared_ptr<Service>& service);

  void createThreadPool(const std::string& name,
                        int port,
                        const TimeoutOption& timeout_opt,
                        size_t threadCount);

  void addForwardTarget(const ForwardTarget& t) {
    forwards_.emplace_back(t);
  }

  void addTask(Event* event);
  void addTask(AsyncClient* client);
  void addTask(const PtrFunc& callback, void* ptr);

  void addEvent(Event* event);
  void forwardEvent(Event* event, const Peer& peer);

  void addCallback(const VoidFunc& callback) {
    acceptor_.add(callback);
  }

  bool waitGroup(const std::vector<Event*>& events) {
    return group_.createGroup(events);
  }

  bool exceedConnectionLimit() const {
    return Socket::count() >= options_.connectionLimit;
  }
  bool exceedFiberLimit() const {
    return Fiber::count() >= options_.fiberLimit;
  }

  void monitoring() const;

private:
  template <class T>
  struct ThreadPoolMap {
    std::map<int, std::shared_ptr<T>> pools;

    void add(int id, size_t threadCount, bool bindCpu = false) {
      auto factory = std::make_shared<ThreadFactory>(
          to<std::string>(typeid(T).name(), id));
      pools.emplace(id, std::make_shared<T>(threadCount, factory, bindCpu));
    }

    T* get(int id) {
      auto it = pools.find(id);
      return it != pools.end() ? it->second.get() : nullptr;
    }
  };

  class Acceptor {
  public:
    Acceptor() {
      loop_.reset(new EventLoop());
    }

    void accept(const Service& service) {
      loop_->listen(service.channel());
    }

    void start() {
      loop_->loop();
      Singleton<Shutdown>::get()->addTask([&]() { loop_->stop(); });
    }

    void add(Event* ev) { loop_->addEvent(ev); }
    void add(const VoidFunc& callback) { loop_->addCallback(callback); }

  private:
    std::unique_ptr<EventLoop> loop_;
  };

  ThreadPool* getPool(int id) {
    ThreadPool* pool = cpuPoolMap_.get(id);
    if (!pool) {
      RDDLOG(FATAL) << "pool[" << id << "] not found";
    }
    return pool;
  }

  Options options_;
  EventGroup group_;
  Acceptor acceptor_;

  //ThreadPoolMap<IOThreadPool> ioPoolMap_;
  ThreadPoolMap<CPUThreadPool> cpuPoolMap_;

  std::map<std::string, std::shared_ptr<Service>> services_;
  std::vector<ForwardTarget> forwards_;
};

}

