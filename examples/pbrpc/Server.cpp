/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/Signal.h"
#include "raster/protocol/proto/AsyncClient.h"
#include "raster/protocol/proto/AsyncServer.h"
#include "accelerator/Logging.h"
#include "accelerator/Portability.h"
#include "accelerator/ScopeGuard.h"
#include "accelerator/Uuid.h"
#include "Proxy.pb.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace raster;
using namespace raster::pbrpc;

class ProxyServiceImpl : public ProxyService {
 public:
  ProxyServiceImpl() {
    ACCLOG(DEBUG) << "ProxyService init";
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

    if (!acc::StringPiece(request->traceid()).startsWith("raster")) {
      response->set_code(ResultCode::E_SOURCE__UNTRUSTED);
      ACCLOG(INFO) << "untrusted request: [" << request->traceid() << "]";
    }
    if (!checkOK(response)) return;

    response->set_traceid(acc::generateUuid(request->traceid(), "rastere"));
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

    ACCTLOG(INFO, request->traceid()) << "query: \"" << request->query() << "\""
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
  gflags::SetVersionString(VERSION);
  gflags::SetUsageMessage("Usage : ./pbrpc");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  auto service = std::make_unique<PBAsyncServer>("Proxy");
  service->addService(std::make_shared<ProxyServiceImpl>());
  acc::Singleton<HubAdaptor>::get()->addService(std::move(service));

  config(FLAGS_conf.c_str(), {
         {configLogging, "logging"},
         {configService, "service"},
         {configThreadPool, "thread"},
         {configNet, "net"},
         {configMonitor, "monitor"},
         {configJobGraph, "job.graph"}
         });

  ACCLOG(INFO) << "raster start ... ^_^";
  acc::Singleton<HubAdaptor>::get()->startService();

  gflags::ShutDownCommandLineFlags();

  return 0;
}
