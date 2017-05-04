/*
 * Copyright (C) 2017, Yeolar
 */

#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include "rddoc/framework/Config.h"
#include "rddoc/io/FileUtil.h"
#include "rddoc/net/Actor.h"
#include "rddoc/plugins/monitor/Monitor.h"
#include "rddoc/util/Logging.h"

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
  opts.rotate  = json::get(j, "rotate", 0);
  Singleton<logging::RDDLogger>::get()->setOptions(opts);
}

void configActor(const dynamic& j) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config actor error: " << j;
    return;
  }
  RDDLOG(INFO) << "config actor";
  Actor::Options opts;
  opts.stack_size   = json::get(j, "stack_size", 64*1024);
  opts.conn_limit   = json::get(j, "conn_limit", 100000);
  opts.task_limit   = json::get(j, "task_limit", 4000);
  opts.poll_size    = json::get(j, "poll_size", 1024);
  opts.poll_timeout = json::get(j, "poll_timeout", 1000);
  opts.forwarding   = json::get(j, "forwarding", false);
  Singleton<Actor>::get()->setOptions(opts);
}

void configTaskThreadPool(const dynamic& j) {
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config thread error: " << j;
    return;
  }
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    RDDLOG(INFO) << "config thread." << k;
    TaskThreadPool::Options thread_opt;
    thread_opt.thread_count       = json::get(v, "thread_count", 4);
    thread_opt.thread_count_limit = json::get(v, "thread_count_limit", 0);
    thread_opt.bindcpu            = json::get(v, "bindcpu", false);
    thread_opt.waiting_task_limit = json::get(v, "waiting_task_limit", 100);
    if (k == "0") {
      Singleton<Actor>::get()->addPool(0, thread_opt);
    } else {
      auto service = json::get(v, "service", "");
      int port = k.asInt();
      if (service == "") {
        RDDLOG(FATAL) << "config thread." << k << " error: " << v;
        return;
      }
      TimeoutOption timeout_opt;
      timeout_opt.ctimeout = json::get(v, "conn_timeout", 100000);
      timeout_opt.rtimeout = json::get(v, "recv_timeout", 300000);
      timeout_opt.wtimeout = json::get(v, "send_timeout", 1000000);
      Singleton<Actor>::get()->configService(
          service, port, timeout_opt, thread_opt);
    }
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

void config(const char* name, std::initializer_list<ConfigTask> confs) {
  RDDLOG(INFO) << "config rdd by conf: " << name;

  std::string s;
  readFile(name, s);
  dynamic j = parseJson(json::stripComments(s));
  if (!j.isObject()) {
    RDDLOG(FATAL) << "config error";
    return;
  }
  RDDLOG(DEBUG) << j;

  for (auto& conf : confs) {
    conf.first(json::resolve(j, conf.second));
  }
}

}

