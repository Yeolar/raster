/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/Actor.h"
#include "rddoc/net/AsyncClient.h"
#include "rddoc/plugins/monitor/Monitor.h"
#include "rddoc/util/Signal.h"

namespace rdd {

void runEventTask(Event* event) {
  {
    auto processor = event->processor(true);
    processor->decodeData(event);
    RDDLOG(V2) << "run process...";
    processor->run();
    RDDLOG(V2) << "run process done";
    processor->encodeData(event);
    event->setType(Event::TOWRITE);
    Singleton<Actor>::get()->addEvent(event);
  }
  exitTask();
}

void runClientTask(AsyncClient* client) {
  yieldTask();
  client->onFinish();
  exitTask();
}

void Actor::start() {
  loop_ = std::unique_ptr<EventLoop>(
      new EventLoop(this, options_.poll_size, options_.poll_timeout));
  for (auto& kv : services_) {
    loop_->listen(kv.second->channel());
  }
  loop_->start();
  Singleton<Shutdown>::get()->addTask([&]() { loop_->stop(); });
}

void Actor::addPool(int pid, const TaskThreadPool::Options& thread_opts) {
  auto pool = std::make_shared<TaskThreadPool>(pid);
  pool->setOptions(thread_opts);
  pool->start();
  pools_.emplace(pid, pool);
}

TaskThreadPool* Actor::getPool(int pid) const {
  auto it = pools_.find(pid);
  return it != pools_.end() ? it->second.get() : nullptr;
}

void Actor::configService(const std::string& name,
                          int port,
                          const TimeoutOption& timeout_opt,
                          const TaskThreadPool::Options& thread_opts) {
  auto service = get_default(services_, name);
  if (service) {
    service->makeChannel(port, timeout_opt);
    addPool(service->channel()->id(), thread_opts);
  } else {
    RDDLOG(FATAL) << "service: [" << name << "] not added";
  }
}

void Actor::addEventTask(Event* event) {
  if (!event->task()) {
    int pid = event->channel()->id();
    TaskThreadPool* pool = getPool(pid);
    if (!pool) {
      RDDLOG(FATAL) << "pool[" << pid << "] not found";
      //delete event;
      return;
    }
    if (exceedTaskLimit() || pool->exceedWaitingTaskLimit()) {
      RDDLOG(WARN) << "pool[" << pid << "] exceed task capacity, drop task";
      delete event;
      return;
    }
    Task* task = new EventTask(event, options_.stack_size, pid);
    task->set(runEventTask, event);
    event->setTask(task);
    RDDLOG(V2) << "pool[" << pid << "] "
      << "add net task(" << (void*)task << ") with ev(" << (void*)event << ")";
    pool->addTask(task);
  }
  else {
    int pid = event->task()->pid();
    TaskThreadPool* pool = getPool(pid);
    if (!pool) {
      RDDLOG(FATAL) << "pool[" << pid << "] not found";
      //delete event;
      return;
    }
    if (group_.finishGroup(event)) {
      RDDLOG(V2) << "pool[" << pid << "] "
        << "re-add net task(" << (void*)event->task() << ") "
        << "with ev(" << (void*)event << ")";
      pool->addTask(event->task());
    }
  }
}

void Actor::addClientTask(AsyncClient* client) {
  int pid = 0;
  TaskThreadPool* pool = getPool(pid);
  if (!pool) {
    RDDLOG(FATAL) << "pool[" << pid << "] not found";
    return;
  }
  if (exceedTaskLimit() || pool->exceedWaitingTaskLimit()) {
    RDDLOG(WARN) << "pool[0] exceed task capacity";
    // still add task
  }
  Task* task = new EventTask(client->event(), options_.stack_size, pid);
  task->set(runClientTask, client);
  client->event()->setTask(task);
  task->addBlockedCallback(std::bind(&Actor::addEvent, this, client->event()));
  RDDLOG(V2) << "pool[0] add asyncclient task(" << (void*)task << ")";
  pool->addTask(task);
}

void Actor::addCallbackTask(const PtrCallback& callback, void* ptr) {
  int pid = 0;
  TaskThreadPool* pool = getPool(pid);
  if (!pool) {
    RDDLOG(FATAL) << "pool[" << pid << "] not found";
    return;
  }
  if (exceedTaskLimit() || pool->exceedWaitingTaskLimit()) {
    RDDLOG(WARN) << "pool[0] exceed task capacity";
    // cannot delete void*, so still add task
  }
  Task* task = new Task(options_.stack_size, pid);
  task->set(callback.target<void(*)(void*)>(), ptr);
  RDDLOG(V2) << "pool[0] add callback task(" << (void*)task << ")";
  pool->addTask(task);
}

void Actor::addEvent(Event* event) {
  if (options_.forwarding && event->role() == Socket::CLIENT) {
    for (auto& f : forwards_) {
      if (f.port == event->channel()->peer().port && f.flow > rand() % 100) {
        forwardEvent(event, f.fpeer);
      }
    }
  }
  loop_->addEvent(event);
}

void Actor::forwardEvent(Event* event, const Peer& peer) {
  auto socket = std::make_shared<Socket>(0);
  if (!(*socket) ||
      !(socket->setReuseAddr()) ||
      // !(socket->setLinger(0)) ||
      !(socket->setTCPNoDelay()) ||
      !(socket->setNonBlocking()) ||
      !(socket->connect(peer))) {
    return;
  }
  Event* evcopy = new Event(event->channel(), socket);
  event->wbuf()->cloneInto(*evcopy->wbuf());
  event->wbuf()->unshare();
  evcopy->setForward();
  evcopy->setType(Event::WRITED);
  loop_->addEvent(evcopy);
}

void Actor::monitoring() const {
  for (auto& kv : pools_) {
    RDDMON_AVG(to<std::string>("freethread.pool-", kv.first),
               kv.second->freeThreadCount());
    RDDMON_AVG(to<std::string>("waitingtask.pool-", kv.first),
               kv.second->waitingTaskCount());
    RDDMON_MAX(to<std::string>("waitingtask.pool-", kv.first, ".max"),
               kv.second->waitingTaskCount());
  }
  RDDMON_AVG("backendgroup", group_.workingGroupCount());
}

}

