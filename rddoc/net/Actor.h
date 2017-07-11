/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/concurrency/CPUThreadPool.h"
#include "rddoc/concurrency/IOThreadPool.h"
#include "rddoc/coroutine/Executor.h"
#include "rddoc/io/event/Event.h"
#include "rddoc/io/event/EventLoop.h"
#include "rddoc/net/Channel.h"
#include "rddoc/net/Processor.h"
#include "rddoc/net/Service.h"
#include "rddoc/parallel/Group.h"
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

  void configService(const std::string& name,
                     int port,
                     const TimeoutOption& timeoutOpt);
  void configThreads(const std::string& name,
                     size_t threadCount,
                     bool bindCpu);

  void addForwardTarget(const ForwardTarget& t) {
    forwards_.emplace_back(t);
  }

  void execute(const ExecutorPtr& executor, int poolId = 0);
  void execute(VoidFunc&& func);
  void execute(Event* event);
  void execute(AsyncClient* client);

  void addEvent(Event* event);
  void forwardEvent(Event* event, const Peer& peer);

  bool waitGroup(const std::vector<Event*>& events) {
    for (auto& event : events) {
      if (event->group() != 0) {
        RDDLOG(WARN) << "create group on grouping Event, giveup";
        return false;
      }
    }
    size_t i = group_.create(events.size());
    for (auto& event : events) {
      event->setGroup(i);
    }
    return true;
  }

  bool exceedConnectionLimit() const {
    return Socket::count() >= options_.connectionLimit;
  }
  bool exceedFiberLimit() const {
    return Fiber::count() >= options_.fiberLimit;
  }

  void monitoring() const;

private:
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

  private:
    std::unique_ptr<EventLoop> loop_;
  };

  void addFiber(Fiber* fiber, int poolId);

  Options options_;
  Group group_;
  Acceptor acceptor_;

  std::shared_ptr<IOThreadPool> ioPool_;
  std::map<int, std::shared_ptr<CPUThreadPool>> cpuPoolMap_;

  std::map<std::string, std::shared_ptr<Service>> services_;
  std::vector<ForwardTarget> forwards_;
};

}

