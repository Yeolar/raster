/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/concurrency/TaskThreadPool.h"
#include "rddoc/net/Channel.h"
#include "rddoc/net/Event.h"
#include "rddoc/net/EventGroup.h"
#include "rddoc/net/EventLoop.h"
#include "rddoc/net/EventTask.h"
#include "rddoc/net/Processor.h"
#include "rddoc/net/Service.h"
#include "rddoc/util/Function.h"
#include "rddoc/util/MapUtil.h"
#include "rddoc/util/Singleton.h"
#include "rddoc/util/SysUtil.h"

namespace rdd {

class TaskThreadPool;
class AsyncClient;

class Actor {
public:
  struct Options {
    size_t stack_size{256*1024};
    size_t conn_limit{100000};
    size_t task_limit{4000};
    uint64_t poll_size{1024};
    uint64_t poll_timeout{1000};
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

  void addPool(int pid, const TaskThreadPool::Options& thread_opts);
  TaskThreadPool* getPool(int pid) const;

  void addService(const std::string& name,
                  const std::shared_ptr<Service>& service) {
    services_.emplace(name, service);
  }
  void configService(const std::string& name,
                     int port,
                     const TimeoutOption& timeout_opt,
                     const TaskThreadPool::Options& thread_opts);

  void addForwardTarget(const ForwardTarget& t) {
    forwards_.push_back(t);
  }

  void addEventTask(Event* event);
  void addClientTask(AsyncClient* client);
  void addCallbackTask(const PtrCallback& callback, void* ptr);

  void addEvent(Event* event);
  void forwardEvent(Event* event, const Peer& peer);

  void addCallback(const VoidCallback& callback) {
    loop_->addCallback(callback);
  }

  bool waitGroup(const std::vector<Event*>& events) {
    return group_.createGroup(events);
  }

  bool exceedConnectionLimit() const {
    return Socket::count() >= options_.conn_limit;
  }
  bool exceedTaskLimit() const {
    return Task::count() >= options_.task_limit;
  }

  void monitoring() const;

private:
  Options options_;
  EventGroup group_;
  std::unique_ptr<EventLoop> loop_;
  std::map<int, std::shared_ptr<TaskThreadPool>> pools_;
  std::map<std::string, std::shared_ptr<Service>> services_;
  std::vector<ForwardTarget> forwards_;
};

}

