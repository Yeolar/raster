/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/Signal.h"
#include "raster/protocol/proto/AsyncClient.h"
#include "raster/protocol/proto/AsyncServer.h"
#include "raster/util/Logging.h"
#include "raster/util/ScopeGuard.h"
#include "raster/util/Uuid.h"
#include "Proxy.pb.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace rdd;
using namespace rdd::pbrpc;

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
      Peer peer;
      peer.setFromIpPort(request->forward());
      Query q;
      q.set_traceid(request->traceid());
      q.set_query(request->query());
      Result r;
      PBAsyncClient<ProxyService::Stub> client(peer);
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

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./pbrpc");
  google::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  auto service = make_unique<PBAsyncServer>("Proxy");
  service->addService(std::make_shared<ProxyServiceImpl>());
  Singleton<HubAdaptor>::get()->addService(std::move(service));

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

  google::ShutDownCommandLineFlags();

  return 0;
}
