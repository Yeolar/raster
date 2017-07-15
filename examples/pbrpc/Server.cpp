/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>
#include "rddoc/framework/Config.h"
#include "rddoc/net/Actor.h"
#include "rddoc/protocol/proto/AsyncClient.h"
#include "rddoc/protocol/proto/AsyncServer.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/ScopeGuard.h"
#include "rddoc/util/Signal.h"
#include "rddoc/util/Uuid.h"
#include "Proxy.pb.h"

static const char* VERSION = "1.0.0";

DEFINE_string(conf, "server.json", "Server config file");

namespace rdd {
namespace pbrpc {

class ProxyServiceImpl : public ProxyService {
public:
  ProxyServiceImpl() {
    RDDLOG(DEBUG) << "ProxyService init";
  }

  void run(::google::protobuf::RpcController* controller,
           const Query* request,
           Result* response,
           ::google::protobuf::Closure* done) {
    SCOPE_EXIT {
      done->Run();
    };

    if (!failReason_.empty()) {
      controller->SetFailed(failReason_);
      return;
    }

    if (!StringPiece(request->traceid()).startsWith("rdd")) {
      response->set_code(ResultCode::E_SOURCE__UNTRUSTED);
      RDDLOG(INFO) << "untrusted request: [" << request->traceid() << "]";
    }
    if (!checkOK(response)) return;

    response->set_traceid(generateUuid(request->traceid(), "rdde"));
    response->set_code(ResultCode::OK);

    if (!request->forward().empty()) {
      Peer peer(request->forward());
      Query q;
      q.set_traceid(request->traceid());
      q.set_query(request->query());
      Result r;
      PBAsyncClient<ProxyService::Stub> client(peer.host, peer.port);
      if (!client.connect() ||
          !client.fetch(&ProxyService::Stub::run, r, q)) {
        response->set_code(ResultCode::E_BACKEND_FAILURE);
      }
    }

    RDDTLOG(INFO, request->traceid()) << "query: \"" << request->query() << "\""
      << " code=" << response->code();
    if (!checkOK(response)) return;
  }

private:
  bool checkOK(Result* response) {
    return response->code() < 1000;
  }

  std::string failReason_;
};

}
}

using namespace rdd;

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./pbrpc");
  google::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  auto service = new PBAsyncServer();
  service->addService(std::make_shared<pbrpc::ProxyServiceImpl>());
  Singleton<Actor>::get()->addService(
      "Proxy", std::shared_ptr<Service>(service));

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
