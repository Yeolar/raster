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
#include "raster/framework/Degrader.h"
#include "raster/framework/FalconSender.h"
#include "raster/framework/Monitor.h"
#include "raster/framework/Sampler.h"
#include "raster/util/Logging.h"
#include "raster/util/ProcessUtil.h"
#include "raster/util/ThreadUtil.h"

namespace rdd {

static dynamic defaultLogging() {
  return dynamic::object
    ("logging", dynamic::object
      ("logFile", "rdd.log")
      ("level", 1)
      ("rotate", 0)
      ("splitSize", 0)
      ("async", true));
}

void configLogging(const dynamic& j, bool reload) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config logging error: " << j;
    return;
  }
  RDDLOG(INFO) << "config logger";
  logging::BaseLogger::Options opts;
  opts.logFile   = json::get(j, "logfile", "rdd.log");
  opts.level     = json::get(j, "level", 1);
  opts.rotate    = json::get(j, "rotate", 0);
  opts.splitSize = json::get(j, "splitsize", 0);
  opts.async     = json::get(j, "async", true);
  Singleton<logging::RDDLogger>::get()->setOptions(opts);
}

static dynamic defaultProcess() {
  return dynamic::object
    ("process", dynamic::object
      ("pidfile", "/tmp/rdd.pid"));
}

void configProcess(const dynamic& j, bool reload) {
  if (reload) return;
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config process error: " << j;
    return;
  }
  RDDLOG(INFO) << "config process";
  auto pidfile = json::get(j, "pidfile", "/tmp/rdd.pid");
  ProcessUtil::writePid(pidfile.c_str(), localThreadId());
}

static dynamic defaultActor() {
  return dynamic::object
    ("actor", dynamic::object
      ("stack_size", 64*1024)
      ("conn_limit", 100000)
      ("task_limit", 4000)
      ("poll_size", 1024)
      ("poll_timeout", 1000)
      ("forwarding", false));
}

void configActor(const dynamic& j, bool reload) {
  if (reload) return;
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

static dynamic defaultService() {
  return dynamic::object
    ("service", dynamic::object
      ("8888", dynamic::object
        ("service", "")
        ("conn_timeout", 100000)
        ("recv_timeout", 300000)
        ("send_timeout", 1000000)));
}

void configService(const dynamic& j, bool reload) {
  if (reload) return;
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

static dynamic defaultThreadPool() {
  return dynamic::object
    ("thread", dynamic::object
      ("io", dynamic::object
        ("thread_count", 4)
        ("bindcpu", false))
      ("0", dynamic::object
        ("thread_count", 4)
        ("bindcpu", false)));
}

void configThreadPool(const dynamic& j, bool reload) {
  if (reload) return;
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

static dynamic defaultNet() {
  return dynamic::object
    ("net", dynamic::object
      ("copy", dynamic::array()));
}

void configNetCopy(const dynamic& j, bool reload) {
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

static dynamic defaultMonitor() {
  return dynamic::object
    ("monitor", dynamic::object
      ("open", false)
      ("prefix", "rdd")
      ("sender", "falcon"));
}

void configMonitor(const dynamic& j, bool reload) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config monitor error: " << j;
    return;
  }
  RDDLOG(INFO) << "config monitor";
  if (json::get(j, "open", false)) {
    Singleton<Monitor>::get()->setPrefix(json::get(j, "prefix", "rdd"));
    if (json::get(j, "sender", "falcon") == "falcon") {
      Singleton<Monitor>::get()->setSender(
          std::unique_ptr<Monitor::Sender>(new FalconSender()));
    }
    Singleton<Monitor>::get()->start();
  } else {
    Singleton<Monitor>::get()->stop();
  }
}

static dynamic defaultDegrader() {
  return dynamic::object
    ("degrader", dynamic::object
      ("count", dynamic::object
        ("open", false)
        ("type", "count")
        ("limit", 0)
        ("gap", 0))
      ("rate", dynamic::object
        ("open", false)
        ("type", "rate")
        ("limit", 0)
        ("gap", 0)));
}

void configDegrader(const dynamic& j, bool reload) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config degrader error: " << j;
    return;
  }
  RDDLOG(INFO) << "config degrader";
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    RDDLOG(INFO) << "config degrader." << k;
    for (auto& i : v) {
      if (json::get(i, "type", "") == "count") {
        auto open = json::get(i, "open", false);
        auto limit = json::get(i, "limit", 0);
        auto gap = json::get(i, "gap", 0);
        Singleton<DegraderManager>::get()->setupDegrader<CountDegrader>(
            k.asString(), open, limit, gap);
      }
      if (json::get(i, "type", "") == "rate") {
        auto open = json::get(i, "open", false);
        auto limit = json::get(i, "limit", 0);
        auto rate = json::get(i, "rate", 0.0);
        Singleton<DegraderManager>::get()->setupDegrader<RateDegrader>(
            k.asString(), open, limit, rate);
      }
      // other types
    }
  }
}

static dynamic defaultSampler() {
  return dynamic::object
    ("sampler", dynamic::object
      ("rdd", dynamic::object
        ("open", false)
        ("type", "percent")
        ("percent", 0.0)));
}

void configSampler(const dynamic& j, bool reload) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config sampler error: " << j;
    return;
  }
  RDDLOG(INFO) << "config sampler";
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    RDDLOG(INFO) << "config sampler." << k;
    for (auto& i : v) {
      if (json::get(i, "type", "") == "percent") {
        auto open = json::get(i, "open", false);
        auto percent = json::get(i, "percent", 0.0);
        Singleton<SamplerManager>::get()->setupSampler<PercentSampler>(
            k.asString(), open, percent);
      }
      // other types
    }
  }
}

static dynamic defaultJob() {
  return dynamic::object
    ("job", dynamic::object
      ("graph", dynamic::object()));
}

void configJobGraph(const dynamic& j, bool reload) {
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

std::string generateDefault() {
  dynamic d = dynamic::object;
  d.update(defaultLogging());
  d.update(defaultProcess());
  d.update(defaultActor());
  d.update(defaultService());
  d.update(defaultThreadPool());
  d.update(defaultNet());
  d.update(defaultMonitor());
  d.update(defaultDegrader());
  d.update(defaultSampler());
  d.update(defaultJob());
  return toPrettyJson(d);
}

void ConfigManager::load() {
  RDDLOG(INFO) << "config rdd by conf: " << conf_;

  std::string s;
  if (!readFile(conf_, s)) {
    RDDLOG(FATAL) << "config error: file read error: " << conf_;
    return;
  }
  dynamic j = parseJson(json::stripComments(s));
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config error: JSON parse error";
    return;
  }
  RDDLOG(DEBUG) << j;

  for (auto& task : tasks_) {
    task.first(json::resolve(j, task.second), inited_);
  }
  inited_ = true;
}

void config(const char* name, std::initializer_list<ConfigTask> confs) {
  ConfigManager* cm = Singleton<ConfigManager>::get();
  cm->setConfFile(name);
  for (auto& conf : confs) {
    cm->addTask(conf);
  }
  cm->load();
}

} // namespace rdd
