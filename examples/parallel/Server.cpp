/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/Signal.h"
#include "raster/parallel/ParallelScheduler.h"
#include "raster/protocol/thrift/AsyncServer.h"
#include "raster/util/Logging.h"
#include "raster/util/Portability.h"
#include "raster/util/ReflectObject.h"
#include "raster/util/ScopeGuard.h"
#include "raster/util/Uuid.h"
#include "gen-cpp/Parallel.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace rdd;
using namespace rdd::parallel;

void parallelFunc(int id) {
  RDDLOG(INFO) << "handle in parallelFunc " << id;
}

class ParallelJob
  : public ReflectObject<JobBase, ParallelJob> {
 public:
  ~ParallelJob() override {}

  void run() override {
    RDDLOG(INFO) << "handle in " << name_;
  }
};

RDD_RF_REG(JobBase, ParallelJob);

class ParallelHandler : virtual public ParallelIf {
 public:
  ParallelHandler() {
    RDDLOG(DEBUG) << "ParallelHandler init";
  }

  void run(Result& _return, const Query& query) {
    if (!StringPiece(query.traceid).startsWith("rdd")) {
      _return.__set_code(ResultCode::E_SOURCE__UNTRUSTED);
      RDDLOG(INFO) << "untrusted request: [" << query.traceid << "]";
    }
    if (!checkOK(_return)) return;

    _return.__set_traceid(generateUuid(query.traceid, "rdde"));
    _return.__set_code(ResultCode::OK);

    // function style parallel executing
    ParallelScheduler scheduler1(
        Singleton<HubAdaptor>::get()->getSharedCPUThreadPoolExecutor(0));
    for (size_t i = 1; i <= 4; i++) {
      scheduler1.add("", {}, std::bind(parallelFunc, i));
    }
    scheduler1.run(true);

    // config job style parallel executing
    ParallelScheduler scheduler2(
        Singleton<HubAdaptor>::get()->getSharedCPUThreadPoolExecutor(0),
        Singleton<GraphManager>::get()->graph("graph1"));
    scheduler2.run(true);

    RDDTLOG(INFO, query.traceid) << "query: \"" << query.query << "\""
      << " code=" << _return.code;
    if (!checkOK(_return)) return;
  }

 private:
  bool checkOK(const Result& result) {
    return result.code < 1000;
  }
};

int main(int argc, char* argv[]) {
  gflags::SetVersionString(VERSION);
  gflags::SetUsageMessage("Usage : ./parallel");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  Singleton<HubAdaptor>::get()->addService(
      make_unique<
        TAsyncServer<ParallelHandler, ParallelProcessor>>("Parallel"));

  config(FLAGS_conf.c_str(), {
         {configLogging, "logging"},
         {configService, "service"},
         {configThreadPool, "thread"},
         {configNet, "net"},
         {configMonitor, "monitor"},
         {configJobGraph, "job.graph"}
         });

  RDDLOG(INFO) << "rdd start ... ^_^";
  Singleton<HubAdaptor>::get()->startService();

  gflags::ShutDownCommandLineFlags();

  return 0;
}
