/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/framework/Monitor.h"
#include "raster/io/event/EventExecutor.h"
#include "raster/net/Actor.h"
#include "raster/net/AsyncClient.h"
#include "raster/net/AsyncClientExecutor.h"

namespace rdd {

void Actor::start() {
  for (auto& kv : services_) {
    acceptor_.accept(*kv.second);
  }
  acceptor_.start();
}

void Actor::addService(const std::string& name,
                       const std::shared_ptr<Service>& service) {
  services_.emplace(name, service);
}

void Actor::configService(const std::string& name,
                          int port,
                          const TimeoutOption& timeoutOpt) {
  auto service = get_default(services_, name);
  if (!service) {
    RDDLOG(FATAL) << "service: [" << name << "] not added";
    return;
  }
  service->makeChannel(port, timeoutOpt);
}

void Actor::configThreads(const std::string& name,
                          size_t threadCount,
                          bool bindCpu) {
  auto factory = std::make_shared<ThreadFactory>(
      (name == "io" ? "IOThreadPool" : "CPUThreadPool" + name) + ':');
  if (name == "io") {
    ioPool_.reset(new IOThreadPool(threadCount, factory, bindCpu));
  } else {
    cpuPoolMap_.emplace(
        to<int>(name),
        std::make_shared<CPUThreadPool>(threadCount, factory, bindCpu));
  }
}

void Actor::execute(VoidFunc&& func) {
  ExecutorPtr executor = ExecutorPtr(new FunctionExecutor(std::move(func)));
  execute(executor);
}

void Actor::execute(Event* event) {
  if (!event->executor()) {
    ExecutorPtr executor = ExecutorPtr(new EventExecutor(event));
    executor->addSchedule(std::bind(&Actor::addEvent, this, event));
    execute(executor, event->channel()->id());
  } else {
    size_t i = event->group();
    event->setGroup(0);
    if (i == 0 || group_.finish(i)) {
      addFiber(event->executor()->fiber, event->channel()->id());
    }
  }
}

void Actor::execute(AsyncClient* client) {
  ExecutorPtr executor = ExecutorPtr(new AsyncClientExecutor(client));
  executor->addCallback(std::bind(&Actor::addEvent, this, client->event()));
  execute(executor);
}

void Actor::execute(const ExecutorPtr& executor, int poolId) {
  Fiber* fiber = executor->fiber;
  if (!fiber) {
    if (exceedFiberLimit()) {
      RDDLOG(WARN) << "exceed fiber capacity";
      // still add fiber
    }
    fiber = new Fiber(options_.stackSize, executor);
  }
  addFiber(fiber, poolId);
}

void Actor::addFiber(Fiber* fiber, int poolId) {
  auto pool = get_default(cpuPoolMap_, poolId);
  if (!pool) {
    pool = get_default(cpuPoolMap_, 0);
    if (!pool) {
      RDDLOG(FATAL) << "CPUThreadPool" << poolId
        << " and CPUThreadPool0 not found";
    }
  }
  RDDLOG(V2) << pool->getThreadFactory()->namePrefix()
    << "* add fiber(" << (void*)fiber << ")"
    << " with executor(" << (void*)fiber->executor().get() << ")";
  pool->add(std::bind(FiberManager::run, fiber));
}

void Actor::addEvent(Event* event) {
  if (options_.forwarding && event->role() == Socket::CLIENT) {
    for (auto& f : forwards_) {
      if (f.port == event->channel()->peer().port && f.flow > rand() % 100) {
        forwardEvent(event, f.fpeer);
      }
    }
  }
  ioPool_->getEventLoop()->addEvent(event);
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
  Event* evcopy = createEvent(event->channel(), socket);
  event->wbuf()->cloneInto(*evcopy->wbuf());
  event->wbuf()->unshare();
  evcopy->setForward();
  evcopy->setType(Event::WRITED);
  ioPool_->getEventLoop()->addEvent(evcopy);
}

void Actor::monitoring() const {
  RDDMON_AVG("totalfiber", Fiber::count());
  RDDMON_AVG("connection", Socket::count());
  RDDMON_AVG("backendgroup", group_.count());
    /*
  for (auto& kv : cpuPools_) {
    RDDMON_AVG(to<std::string>("freethread.pool-", kv.first),
               kv.second->freeThreadCount());
    RDDMON_AVG(to<std::string>("waitingfiber.pool-", kv.first),
               kv.second->waitingFiberCount());
    RDDMON_MAX(to<std::string>("waitingfiber.pool-", kv.first, ".max"),
               kv.second->waitingFiberCount());
  }
               */
}

} // namespace rdd
