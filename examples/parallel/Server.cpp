/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/Signal.h"
#include "accelerator/parallel/ParallelScheduler.h"
#include "raster/protocol/thrift/AsyncServer.h"
#include "accelerator/Logging.h"
#include "accelerator/Portability.h"
#include "accelerator/ReflectObject.h"
#include "accelerator/ScopeGuard.h"
#include "accelerator/Uuid.h"
#include "gen-cpp/Parallel.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace acc;
using namespace rdd;

void parallelFunc(int id) {
  ACCLOG(INFO) << "handle in parallelFunc " << id;
}

class ParallelJob
  : public acc::ReflectObject<acc::JobBase, ParallelJob> {
 public:
  ~ParallelJob() override {}

  void run() override {
    ACCLOG(INFO) << "handle in " << name_;
  }
};

ACC_RF_REG(JobBase, ParallelJob);

class ParallelHandler : virtual public acc::ParallelIf {
 public:
  ParallelHandler() {
    ACCLOG(DEBUG) << "ParallelHandler init";
  }

  void run(Result& _return, const Query& query) {
    if (!acc::StringPiece(query.traceid).startsWith("rdd")) {
      _return.__set_code(ResultCode::E_SOURCE__UNTRUSTED);
      ACCLOG(INFO) << "untrusted request: [" << query.traceid << "]";
    }
    if (!checkOK(_return)) return;

    _return.__set_traceid(acc::generateUuid(query.traceid, "rdde"));
    _return.__set_code(ResultCode::OK);

    // function style parallel executing
    acc::ParallelScheduler scheduler1(
        acc::Singleton<HubAdaptor>::get()->getSharedCPUThreadPoolExecutor(0));
    for (size_t i = 1; i <= 4; i++) {
      scheduler1.add("", {}, std::bind(parallelFunc, i));
    }
    scheduler1.run(true);

    // config job style parallel executing
    acc::ParallelScheduler scheduler2(
        acc::Singleton<HubAdaptor>::get()->getSharedCPUThreadPoolExecutor(0),
        acc::Singleton<acc::GraphManager>::get()->graph("graph1"));
    scheduler2.run(true);

    ACCTLOG(INFO, query.traceid) << "query: \"" << query.query << "\""
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

  acc::Singleton<HubAdaptor>::get()->addService(
      acc::make_unique<
        TAsyncServer<ParallelHandler, ParallelProcessor>>("Parallel"));

  config(FLAGS_conf.c_str(), {
         {configLogging, "logging"},
         {configService, "service"},
         {configThreadPool, "thread"},
         {configNet, "net"},
         {configMonitor, "monitor"},
         {configJobGraph, "job.graph"}
         });

  ACCLOG(INFO) << "rdd start ... ^_^";
  acc::Singleton<HubAdaptor>::get()->startService();

  gflags::ShutDownCommandLineFlags();

  return 0;
}
