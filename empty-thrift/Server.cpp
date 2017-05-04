/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>
#include "rddoc/framework/Config.h"
#include "rddoc/net/Actor.h"
#include "rddoc/protocol/thrift/AsyncServer.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/ScopeGuard.h"
#include "rddoc/util/Signal.h"
#include "rddoc/util/Uuid.h"
#include "gen-cpp/Empty.h"

static const char* VERSION = "1.0.0";

DEFINE_string(conf, "server.json", "Server config file");

namespace rdd {
namespace empty {

class EmptyHandler : virtual public EmptyIf {
public:
  EmptyHandler() {
    RDDLOG(DEBUG) << "EmptyHandler init";
  }

  void run(Result& _return, const Query& query) {
    if (!StringPiece(query.traceid).startsWith("rdd")) {
      _return.__set_code(ResultCode::E_SOURCE__UNTRUSTED);
      RDDLOG(INFO) << "untrusted request: [" << query.traceid << "]";
    }
    if (!checkOK(_return)) return;

    _return.__set_traceid(generateUuid(query.traceid, "rdde"));
    _return.__set_code(ResultCode::OK);
  }

private:
  bool checkOK(const Result& result) {
    return result.code < 1000;
  }
};

}
}

using namespace rdd;

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./empty");
  google::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  std::shared_ptr<Service> empty(
      new TAsyncServer<empty::EmptyHandler, empty::EmptyProcessor>());
  Singleton<Actor>::get()->addService("Empty", empty);

  config(FLAGS_conf.c_str(), {
         {configLogging, "logging"},
         {configActor, "actor"},
         {configTaskThreadPool, "thread"},
         {configNetCopy, "net.copy"},
         {configMonitor, "monitor"}
         });

  RDDLOG(INFO) << "rdd start ... ^_^";
  Singleton<Actor>::get()->start();

  google::ShutDownCommandLineFlags();

  return 0;
}
