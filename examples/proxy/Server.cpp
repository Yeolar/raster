/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/Signal.h"
#include "raster/protocol/thrift/AsyncClient.h"
#include "raster/protocol/thrift/AsyncServer.h"
#include "accelerator/Logging.h"
#include "accelerator/Portability.h"
#include "accelerator/ScopeGuard.h"
#include "accelerator/Uuid.h"
#include "gen-cpp/Proxy.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace rdd;
using namespace rdd::proxy;

class ProxyHandler : virtual public ProxyIf {
 public:
  ProxyHandler() {
    ACCLOG(DEBUG) << "ProxyHandler init";
  }

  void run(Result& _return, const Query& query) {
    if (!acc::StringPiece(query.traceid).startsWith("rdd")) {
      _return.__set_code(ResultCode::E_SOURCE__UNTRUSTED);
      ACCLOG(INFO) << "untrusted request: [" << query.traceid << "]";
    }
    if (!checkOK(_return)) return;

    _return.__set_traceid(acc::generateUuid(query.traceid, "rdde"));
    _return.__set_code(ResultCode::OK);

    if (!query.forward.empty()) {
      Peer peer;
      peer.setFromIpPort(query.forward);
      Query q;
      q.__set_traceid(query.traceid);
      q.__set_query(query.query);
      TAsyncClient<ProxyClient> client(peer);
      client.setKeepAlive();
      if (!client.connect() ||
          !client.fetch(&ProxyClient::recv_run, _return,
                        &ProxyClient::send_run, q)) {
        _return.__set_code(ResultCode::E_BACKEND_FAILURE);
      }
    }

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
  gflags::SetUsageMessage("Usage : ./proxy");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  acc::Singleton<HubAdaptor>::get()->addService(
      acc::make_unique<TAsyncServer<ProxyHandler, ProxyProcessor>>("Proxy"));

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
