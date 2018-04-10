/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/Signal.h"
#include "raster/protocol/thrift/AsyncServer.h"
#include "accelerator/Logging.h"
#include "accelerator/Portability.h"
#include "accelerator/ScopeGuard.h"
#include "accelerator/Uuid.h"
#include "gen-cpp/Empty.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace rdd;
using namespace rdd::empty;

class EmptyHandler : virtual public EmptyIf {
 public:
  EmptyHandler() {
    ACCLOG(DEBUG) << "EmptyHandler init";
  }

  void run(Result& _return, const Query& query) {
    if (!acc::StringPiece(query.traceid).startsWith("rdd")) {
      _return.__set_code(ResultCode::E_SOURCE__UNTRUSTED);
      ACCLOG(INFO) << "untrusted request: [" << query.traceid << "]";
    }
    if (!checkOK(_return)) return;

    _return.__set_traceid(acc::generateUuid(query.traceid, "rdde"));
    _return.__set_code(ResultCode::OK);

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
  gflags::SetUsageMessage("Usage : ./empty");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  acc::Singleton<HubAdaptor>::get()->addService(
      acc::make_unique<TAsyncServer<EmptyHandler, EmptyProcessor>>("Empty"));

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
