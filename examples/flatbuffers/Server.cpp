/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/Signal.h"
#include "raster/protocol/binary/AsyncClient.h"
#include "raster/protocol/binary/AsyncServer.h"
#include "raster/util/Logging.h"
#include "raster/util/ScopeGuard.h"
#include "raster/util/Uuid.h"
#include "Helper.h"
#include "table_generated.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace rdd;
using namespace rdd::fbs;

class Proxy : public BinaryProcessor {
 public:
  Proxy(Event* event) : BinaryProcessor(event) {
    RDDLOG(DEBUG) << "Proxy init";
  }

  void process(ByteRange& response, const ByteRange& request) {
    auto query = ::flatbuffers::GetRoot<Query>(request.data());
    DCHECK(verifyFlatbuffer(query, request));

    if (!StringPiece(query->traceid()->str()).startsWith("rdd")) {
      RDDLOG(INFO) << "untrusted request: [" << query->traceid() << "]";
      ::flatbuffers::FlatBufferBuilder fbb;
      fbb.Finish(CreateResult(fbb, 0, ResultCode_E_SOURCE__UNTRUSTED));
      response.reset(fbb.GetBufferPointer(), fbb.GetSize());
      return;
    }

    auto traceid = generateUuid(query->traceid()->str(), "rdde");
    ResultCode code = ResultCode_OK;

    if (query->forward()->Length() != 0) {
      Peer peer;
      peer.setFromIpPort(query->forward()->str());
      BinaryAsyncClient client(peer);
      ::flatbuffers::FlatBufferBuilder fbb;
      fbb.Finish(
          CreateQuery(fbb,
                      fbb.CreateString(query->traceid()),
                      fbb.CreateString(query->query()),
                      0));
      ByteRange q(fbb.GetBufferPointer(), fbb.GetSize());
      ByteRange r;
      if (!client.connect() || !client.fetch(r, q)) {
        code = ResultCode_E_BACKEND_FAILURE;
      }
    }

    RDDTLOG(INFO, traceid) << "query: \"" << query->query()->str() << "\""
      << " code=" << code;
    ::flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(CreateResult(fbb, fbb.CreateString(traceid), code));
    response.reset(fbb.GetBufferPointer(), fbb.GetSize());
  }
};

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./flatbuffers");
  google::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  Singleton<HubAdaptor>::get()->addService(
      make_unique<BinaryAsyncServer<Proxy>>("Proxy"));

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
