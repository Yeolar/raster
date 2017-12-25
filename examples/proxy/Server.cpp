/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>
#include "raster/framework/Config.h"
#include "raster/framework/Signal.h"
#include "raster/net/Actor.h"
#include "raster/protocol/thrift/AsyncClient.h"
#include "raster/protocol/thrift/AsyncServer.h"
#include "raster/util/Logging.h"
#include "raster/util/ScopeGuard.h"
#include "raster/util/Uuid.h"
#include "gen-cpp/Proxy.h"

static const char* VERSION = "1.0.0";

DEFINE_string(conf, "server.json", "Server config file");

namespace rdd {
namespace proxy {

class ProxyHandler : virtual public ProxyIf {
public:
  ProxyHandler() {
    RDDLOG(DEBUG) << "ProxyHandler init";
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
  google::SetUsageMessage("Usage : ./proxy");
  google::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  std::shared_ptr<Service> proxy(
      new TAsyncServer<proxy::ProxyHandler, proxy::ProxyProcessor>());
  Singleton<Actor>::get()->addService("Proxy", proxy);

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
