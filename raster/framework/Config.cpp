/*
 * Copyright (C) 2017, Yeolar
 */

#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include "raster/framework/Config.h"
#include "raster/io/FileUtil.h"
#include "raster/net/Actor.h"
#include "raster/parallel/Scheduler.h"
#include "raster/framework/Monitor.h"
#include "raster/util/Logging.h"

namespace rdd {

void configLogging(const dynamic& j) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config logging error: " << j;
    return;
  }
  RDDLOG(INFO) << "config logger";
  logging::BaseLogger::Options opts;
  opts.logfile = json::get(j, "logfile", "rdd.log");
  opts.level   = json::get(j, "level", 1);
  opts.async   = json::get(j, "async", true);
  Singleton<logging::RDDLogger>::get()->setOptions(opts);
}

void configActor(const dynamic& j) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config actor error: " << j;
    return;
  }
  RDDLOG(INFO) << "config actor";
  Actor::Options opts;
  opts.stackSize       = json::get(j, "stack_size", 64*1024);
  opts.connectionLimit = json::get(j, "conn_limit", 100000);
  opts.fiberLimit      = json::get(j, "task_limit", 4000);
  opts.pollSize        = json::get(j, "poll_size", 1024);
  opts.pollTimeout     = json::get(j, "poll_timeout", 1000);
  opts.forwarding      = json::get(j, "forwarding", false);
  Singleton<Actor>::get()->setOptions(opts);
}

void configService(const dynamic& j) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config service error: " << j;
    return;
  }
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    RDDLOG(INFO) << "config service." << k;
    auto service = json::get(v, "service", "");
    int port = k.asInt();
    TimeoutOption timeoutOpt;
    timeoutOpt.ctimeout = json::get(v, "conn_timeout", 100000);
    timeoutOpt.rtimeout = json::get(v, "recv_timeout", 300000);
    timeoutOpt.wtimeout = json::get(v, "send_timeout", 1000000);
    Singleton<Actor>::get()->configService(service, port, timeoutOpt);
  }
}

void configThreadPool(const dynamic& j) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config thread error: " << j;
    return;
  }
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    RDDLOG(INFO) << "config thread." << k;
    auto name = k.asString();
    int threadCount = json::get(v, "thread_count", 4);
    bool bindCpu = json::get(v, "bind_cpu", false);
    Singleton<Actor>::get()->configThreads(name, threadCount, bindCpu);
  }
}

void configNetCopy(const dynamic& j) {
  if (!j.isArray()) {
    RDDLOG(FATAL) << "config net.copy error: " << j;
    return;
  }
  RDDLOG(INFO) << "config net.copy";
  for (auto& i : j) {
    Actor::ForwardTarget t;
    t.port  = json::get(i, "port", 0);
    t.fpeer = {json::get(i, "fhost", ""), json::get(i, "fport", 0)};
    t.flow  = json::get(i, "flow", 100);
    Singleton<Actor>::get()->addForwardTarget(t);
  }
}

void configMonitor(const dynamic& j) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config monitor error: " << j;
    return;
  }
  RDDLOG(INFO) << "config monitor";
  if (json::get(j, "open", false)) {
    Singleton<Monitor>::get()->setPrefix(json::get(j, "prefix", "rdd"));
    Singleton<Monitor>::get()->start();
  }
}

void configJobGraph(const dynamic& j) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config job.graph error: " << j;
    return;
  }
  RDDLOG(INFO) << "config job.graph";
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    RDDLOG(INFO) << "config job.graph." << k;
    Graph& g = Singleton<GraphManager>::get()->getGraph(k.asString());
    for (auto& i : v) {
      auto name = json::get(i, "name", "");
      auto next = json::getArray<std::string>(i, "next");
      g.add(name, next);
    }
  }
}

void config(const char* name, std::initializer_list<ConfigTask> confs) {
  RDDLOG(INFO) << "config rdd by conf: " << name;

  std::string s;
  if (!readFile(name, s)) {
    RDDLOG(FATAL) << "config error: file read error: " << name;
    return;
  }
  dynamic j = parseJson(json::stripComments(s));
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config error: JSON parse error";
    return;
  }
  RDDLOG(DEBUG) << j;

  for (auto& conf : confs) {
    conf.first(json::resolve(j, conf.second));
  }
}

} // namespace rdd
