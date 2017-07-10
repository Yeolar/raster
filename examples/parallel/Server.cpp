/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>
#include "rddoc/framework/Config.h"
#include "rddoc/net/Actor.h"
#include "rddoc/parallel/Scheduler.h"
#include "rddoc/protocol/thrift/AsyncClient.h"
#include "rddoc/protocol/thrift/AsyncServer.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/ReflectObject.h"
#include "rddoc/util/ScopeGuard.h"
#include "rddoc/util/Signal.h"
#include "rddoc/util/Uuid.h"
#include "gen-cpp/Empty.h"

static const char* VERSION = "1.0.0";

DEFINE_string(conf, "server.json", "Server config file");

namespace rdd {

class EmptyJobExecutor : public ReflectObject<JobExecutor, EmptyJobExecutor> {
public:
  struct MyContext : public Context {
    empty::Result* result;
  };

  virtual ~EmptyJobExecutor() {}

  void handle() {
    if (context_) {
      RDDLOG(INFO) << "handle in " << name_ << ": code="
        << std::dynamic_pointer_cast<MyContext>(context_)->result->code;
    } else {
      RDDLOG(INFO) << "handle in " << name_;
    }
  }
};
RDD_RF_REG(JobExecutor, EmptyJobExecutor);

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

    if (!query.forward.empty()) {
      Peer peer(query.forward);
      Query q;
      q.__set_traceid(query.traceid);
      q.__set_query(query.query);
      TAsyncClient<EmptyClient> client(peer.host, peer.port);
      client.setKeepAlive();
      if (!client.connect() ||
          !client.fetch(&EmptyClient::recv_run, _return,
                        &EmptyClient::send_run, q)) {
        _return.__set_code(ResultCode::E_BACKEND_FAILURE);
      }
    }

    auto jobctx = new EmptyJobExecutor::MyContext();
    jobctx->result = &_return;
    make_unique<Scheduler>("graph1", JobExecutor::ContextPtr(jobctx))->run();

    RDDTLOG(INFO, query.traceid) << "query: \"" << query.query << "\""
      << " code=" << _return.code;
    if (!checkOK(_return)) return;
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
         {configService, "service"},
         {configThreadPool, "thread"},
         {configNetCopy, "net.copy"},
         {configMonitor, "monitor"},
         {configJobGraph, "job.graph"}
         });

  RDDLOG(INFO) << "rdd start ... ^_^";
  Singleton<Actor>::get()->start();

  google::ShutDownCommandLineFlags();

  return 0;
}
