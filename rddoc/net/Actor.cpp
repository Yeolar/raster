/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/Actor.h"
#include "rddoc/net/AsyncClient.h"
#include "rddoc/plugins/monitor/Monitor.h"

namespace rdd {

void eventRunner(Event* event) {
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
  FiberManager::exit();
}

void clientRunner(AsyncClient* client) {
  FiberManager::yield();
  if (client->callbackMode()) {
    client->callback();
    delete client;
  }
  FiberManager::exit();
}

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

void Actor::createThreadPool(const std::string& name,
                             int port,
                             const TimeoutOption& timeoutOpt,
                             size_t threadCount) {
  if (port == 0) {
    cpuPoolMap_.add(0, threadCount);
    return;
  }
  auto service = get_default(services_, name);
  if (!service) {
    RDDLOG(FATAL) << "service: [" << name << "] not added";
    return;
  }
  service->makeChannel(port, timeoutOpt);
  cpuPoolMap_.add(service->channel()->id(), threadCount);
}

void Actor::addTask(Event* event) {
  int id = event->channel()->id();
  ThreadPool* pool = getPool(id);
  if (!event->fiber()) {
    if (exceedFiberLimit()) {
      RDDLOG(WARN) << "pool[" << id << "] exceed fiber capacity, drop fiber";
      delete event;
      return;
    }
    Fiber* fiber = new Fiber(options_.stackSize);
    fiber->set(eventRunner, event);
    fiber->setData(event);
    event->setFiber(fiber);
    RDDLOG(V2) << "pool[" << id << "] "
      << "add fiber(" << (void*)fiber << ") with ev(" << (void*)event << ")";
    pool->add(std::bind(FiberManager::run, fiber));
  }
  else {
    if (group_.finishGroup(event)) {
      RDDLOG(V2) << "pool[" << id << "] "
        << "re-add fiber(" << (void*)event->fiber() << ") "
        << "with ev(" << (void*)event << ")";
      pool->add(std::bind(FiberManager::run, event->fiber()));
    }
  }
}

void Actor::addTask(AsyncClient* client) {
  ThreadPool* pool = getPool(0);
  if (exceedFiberLimit()) {
    RDDLOG(WARN) << "pool[0] exceed fiber capacity";
    // still add fiber
  }
  Fiber* fiber = new Fiber(options_.stackSize);
  fiber->set(clientRunner, client);
  fiber->setData(client->event());
  client->event()->setFiber(fiber);
  fiber->addBlockedCallback(std::bind(&Actor::addEvent, this, client->event()));
  RDDLOG(V2) << "pool[0] add asyncclient fiber(" << (void*)fiber << ")";
  pool->add(std::bind(FiberManager::run, fiber));
}

void Actor::addTask(const PtrFunc& callback, void* ptr) {
  ThreadPool* pool = getPool(0);
  if (exceedFiberLimit()) {
    RDDLOG(WARN) << "pool[0] exceed fiber capacity";
    // cannot delete void*, so still add fiber
  }
  Fiber* fiber = new Fiber(options_.stackSize);
  fiber->set(callback.target<void(*)(void*)>(), ptr);
  RDDLOG(V2) << "pool[0] add callback fiber(" << (void*)fiber << ")";
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
  acceptor_.add(event);
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
  acceptor_.add(evcopy);
}

void Actor::monitoring() const {
  RDDMON_AVG("totalfiber", Fiber::count());
  RDDMON_AVG("connection", Socket::count());
  RDDMON_AVG("backendgroup", group_.workingGroupCount());
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

}

