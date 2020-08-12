/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/framework/Config.h"

#include <map>
#include <set>
#include <string>
#include <typeinfo>

#include <accelerator/FileUtil.h>
#include <accelerator/Logging.h>
#include <accelerator/Monitor.h>
#include <accelerator/thread/ThreadId.h>

#include "raster/framework/Degrader.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/ProcessUtil.h"
#include "raster/framework/Sampler.h"
#include "raster/framework/json.h"
//#include "raster/framework/FalconSender.h"

namespace raster {

using acc::dynamic;

static dynamic defaultLogging() {
  return dynamic::object
    ("logging", dynamic::object
      ("logFile", "raster.log")
      ("level", 1)
      ("rotate", 0)
      ("splitSize", 0)
      ("async", true));
}

void configLogging(const dynamic& j, bool reload) {
  // reloadable
  if (!j.isObject()) {
    ACCLOG(FATAL) << "config logging error: " << j;
    return;
  }
  ACCLOG(INFO) << "config logger";
  acc::logging::BaseLogger::Options opts;
  opts.logFile   = acc::json::get(j, "logfile", "raster.log");
  opts.level     = acc::json::get(j, "level", 1);
  opts.rotate    = acc::json::get(j, "rotate", 0);
  opts.splitSize = acc::json::get(j, "splitsize", 0);
  opts.async     = acc::json::get(j, "async", true);
  acc::Singleton<acc::logging::ACCLogger>::get()->setOptions(opts);
}

static dynamic defaultProcess() {
  return dynamic::object
    ("process", dynamic::object
      ("pidfile", "/tmp/raster.pid"));
}

void configProcess(const dynamic& j, bool reload) {
  if (reload) return;
  if (!j.isObject()) {
    ACCLOG(FATAL) << "config process error: " << j;
    return;
  }
  ACCLOG(INFO) << "config process";
  auto pidfile = acc::json::get(j, "pidfile", "/tmp/raster.pid");
  acc::writePid(pidfile.c_str(), acc::getOSThreadID());
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
    ACCLOG(FATAL) << "config service error: " << j;
    return;
  }
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    ACCLOG(INFO) << "config service." << k;
    auto service = acc::json::get(v, "service", "");
    int port = k.asInt();
    TimeoutOption timeoutOpt;
    timeoutOpt.ctimeout = acc::json::get(v, "conn_timeout", 100000);
    timeoutOpt.rtimeout = acc::json::get(v, "recv_timeout", 300000);
    timeoutOpt.wtimeout = acc::json::get(v, "send_timeout", 1000000);
    acc::Singleton<HubAdaptor>::get()->configService(service, port, timeoutOpt);
  }
}

static dynamic defaultThreadPool() {
  return dynamic::object
    ("thread", dynamic::object
      ("io", dynamic::object
        ("thread_count", 4))
      ("0", dynamic::object
        ("thread_count", 4)));
}

void configThreadPool(const dynamic& j, bool reload) {
  if (reload) return;
  if (!j.isObject()) {
    ACCLOG(FATAL) << "config thread error: " << j;
    return;
  }
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    ACCLOG(INFO) << "config thread." << k;
    auto name = k.asString();
    int threadCount = acc::json::get(v, "thread_count", 4);
    acc::Singleton<HubAdaptor>::get()->configThreads(name, threadCount);
  }
}

static dynamic defaultNet() {
  return dynamic::object
    ("net", dynamic::object
      ("forwarding", false)
      ("copy", dynamic::array()));
}

void configNet(const dynamic& j, bool reload) {
  // reloadable
  if (!j.isObject()) {
    ACCLOG(FATAL) << "config net error: " << j;
    return;
  }
  ACCLOG(INFO) << "config net";
  auto forwarding = acc::json::get(j, "forwarding", false);
  acc::Singleton<HubAdaptor>::get()->setForwarding(forwarding);
  for (auto& i : j.getDefault("copy", dynamic::array)) {
    ForwardTarget t;
    t.port  = acc::json::get(i, "port", 0);
    t.fpeer = Peer(acc::json::get(i, "fhost", ""), acc::json::get(i, "fport", 0));
    t.flow  = acc::json::get(i, "flow", 100);
    acc::Singleton<HubAdaptor>::get()->addForwardTarget(std::move(t));
  }
}

static dynamic defaultMonitor() {
  return dynamic::object
    ("monitor", dynamic::object
      ("open", false)
      ("prefix", "raster")
      ("sender", "falcon"));
}

void configMonitor(const dynamic& j, bool reload) {
  // reloadable
  /*
  if (!j.isObject()) {
    ACCLOG(FATAL) << "config monitor error: " << j;
    return;
  }
  ACCLOG(INFO) << "config monitor";
  if (acc::json::get(j, "open", false)) {
    acc::Singleton<acc::Monitor>::get()->setPrefix(acc::json::get(j, "prefix", "raster"));
    if (acc::json::get(j, "sender", "falcon") == "falcon") {
      acc::Singleton<acc::Monitor>::get()->setSender(
          std::unique_ptr<acc::Monitor::Sender>(new FalconSender()));
    }
    acc::Singleton<acc::Monitor>::get()->start();
  } else {
    acc::Singleton<acc::Monitor>::get()->stop();
  }
  */
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
  // reloadable
  if (!j.isObject()) {
    ACCLOG(FATAL) << "config degrader error: " << j;
    return;
  }
  ACCLOG(INFO) << "config degrader";
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    ACCLOG(INFO) << "config degrader." << k;
    for (auto& i : v) {
      if (acc::json::get(i, "type", "") == "count") {
        auto open = acc::json::get(i, "open", false);
        auto limit = acc::json::get(i, "limit", 0);
        auto gap = acc::json::get(i, "gap", 0);
        acc::Singleton<DegraderManager>::get()->setupDegrader<CountDegrader>(
            k.asString(), open, limit, gap);
      }
      if (acc::json::get(i, "type", "") == "rate") {
        auto open = acc::json::get(i, "open", false);
        auto limit = acc::json::get(i, "limit", 0);
        auto rate = acc::json::get(i, "rate", 0.0);
        acc::Singleton<DegraderManager>::get()->setupDegrader<RateDegrader>(
            k.asString(), open, limit, rate);
      }
      // other types
    }
  }
}

static dynamic defaultSampler() {
  return dynamic::object
    ("sampler", dynamic::object
      ("raster", dynamic::object
        ("open", false)
        ("type", "percent")
        ("percent", 0.0)));
}

void configSampler(const dynamic& j, bool reload) {
  // reloadable
  if (!j.isObject()) {
    ACCLOG(FATAL) << "config sampler error: " << j;
    return;
  }
  ACCLOG(INFO) << "config sampler";
  for (auto& kv : j.items()) {
    const dynamic& k = kv.first;
    const dynamic& v = kv.second;
    ACCLOG(INFO) << "config sampler." << k;
    for (auto& i : v) {
      if (acc::json::get(i, "type", "") == "percent") {
        auto open = acc::json::get(i, "open", false);
        auto percent = acc::json::get(i, "percent", 0.0);
        acc::Singleton<SamplerManager>::get()->setupSampler<PercentSampler>(
            k.asString(), open, percent);
      }
      // other types
    }
  }
}

std::string generateDefault() {
  dynamic d = dynamic::object;
  d.update(defaultLogging());
  d.update(defaultProcess());
  d.update(defaultService());
  d.update(defaultThreadPool());
  d.update(defaultNet());
  d.update(defaultMonitor());
  d.update(defaultDegrader());
  d.update(defaultSampler());
  return toPrettyJson(d);
}

void ConfigManager::load() {
  ACCLOG(INFO) << "config raster by conf: " << conf_;

  std::string s;
  if (!acc::readFile(conf_, s)) {
    ACCLOG(FATAL) << "config error: file read error: " << conf_;
    return;
  }
  dynamic j = acc::parseJson(acc::json::stripComments(s));
  if (!j.isObject()) {
    ACCLOG(FATAL) << "config error: JSON parse error";
    return;
  }
  ACCLOG(DEBUG) << j;

  for (auto& task : tasks_) {
    task.first(acc::json::resolve(j, task.second), inited_);
  }
  inited_ = true;
}

void config(const char* name, std::initializer_list<ConfigTask> confs) {
  ConfigManager* cm = acc::Singleton<ConfigManager>::get();
  cm->setConfFile(name);
  for (auto& conf : confs) {
    cm->addTask(conf);
  }
  cm->load();
}

} // namespace raster
