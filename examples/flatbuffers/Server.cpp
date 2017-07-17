/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>
#include "Encoding.h"
#include "rddoc/framework/Config.h"
#include "rddoc/net/Actor.h"
#include "rddoc/protocol/binary/AsyncClient.h"
#include "rddoc/protocol/binary/AsyncServer.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/ScopeGuard.h"
#include "rddoc/util/Signal.h"
#include "rddoc/util/Uuid.h"
#include "table_generated.h"

static const char* VERSION = "1.0.0";

DEFINE_string(conf, "server.json", "Server config file");

namespace rdd {
namespace fbs {

class Proxy : public BinaryProcessor<std::vector<uint8_t>,
                                     ::flatbuffers::FlatBufferBuilder> {
public:
  Proxy() {
    RDDLOG(DEBUG) << "Proxy init";
  }

  bool process(::flatbuffers::FlatBufferBuilder& response,
               const std::vector<uint8_t>& request) {
    auto query = ::flatbuffers::GetRoot<Query>(&request[0]);
    if (!StringPiece(query->traceid()->str()).startsWith("rdd")) {
      RDDLOG(INFO) << "untrusted request: [" << query->traceid() << "]";
      response.Finish(
          CreateResult(response, 0, ResultCode_E_SOURCE__UNTRUSTED));
      return true;
    }

    auto traceid = generateUuid(query->traceid()->str(), "rdde");
    ResultCode code = ResultCode_OK;

    if (query->forward()->Length() != 0) {
      Peer peer(query->forward()->str());
      ::flatbuffers::FlatBufferBuilder q;
      q.Finish(
          CreateQuery(q,
                      q.CreateString(query->traceid()),
                      q.CreateString(query->query()),
                      0));
      BinaryAsyncClient client(peer.host, peer.port);
      std::vector<uint8_t> r;
      if (!client.connect() || !client.fetch(r, q)) {
        code = ResultCode_E_BACKEND_FAILURE;
      }
    }

    RDDTLOG(INFO, traceid) << "query: \"" << query->query()->str() << "\""
      << " code=" << code;
    response.Finish(
        CreateResult(response, response.CreateString(traceid), code));
    return true;
  }
};

}
}

using namespace rdd;

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./flatbuffers");
  google::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  std::shared_ptr<Service> proxy(
      new BinaryAsyncServer<fbs::Proxy>());
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
