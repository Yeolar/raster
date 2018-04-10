/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/framework/Signal.h"
#include "raster/protocol/binary/AsyncClient.h"
#include "raster/protocol/binary/AsyncServer.h"
#include "accelerator/Logging.h"
#include "accelerator/Portability.h"
#include "accelerator/ScopeGuard.h"
#include "accelerator/Uuid.h"
#include "Helper.h"
#include "table_generated.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace rdd;
using namespace rdd::fbs;

class Proxy : public BinaryProcessor {
 public:
  Proxy(Event* event) : BinaryProcessor(event) {
    ACCLOG(DEBUG) << "Proxy init";
  }

  void process(acc::ByteRange& response, const acc::ByteRange& request) {
    auto query = ::flatbuffers::GetRoot<Query>(request.data());
    DCHECK(verifyFlatbuffer(query, request));

    if (!acc::StringPiece(query->traceid()->str()).startsWith("rdd")) {
      ACCLOG(INFO) << "untrusted request: [" << query->traceid() << "]";
      ::flatbuffers::FlatBufferBuilder fbb;
      fbb.Finish(CreateResult(fbb, 0, ResultCode_E_SOURCE__UNTRUSTED));
      response.reset(fbb.GetBufferPointer(), fbb.GetSize());
      return;
    }

    auto traceid = acc::generateUuid(query->traceid()->str(), "rdde");
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
      acc::ByteRange q(fbb.GetBufferPointer(), fbb.GetSize());
      acc::ByteRange r;
      if (!client.connect() || !client.fetch(r, q)) {
        code = ResultCode_E_BACKEND_FAILURE;
      }
    }

    ACCTLOG(INFO, traceid) << "query: \"" << query->query()->str() << "\""
      << " code=" << code;
    ::flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(CreateResult(fbb, fbb.CreateString(traceid), code));
    response.reset(fbb.GetBufferPointer(), fbb.GetSize());
  }
};

int main(int argc, char* argv[]) {
  gflags::SetVersionString(VERSION);
  gflags::SetUsageMessage("Usage : ./flatbuffers");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  acc::Singleton<HubAdaptor>::get()->addService(
      acc::make_unique<BinaryAsyncServer<Proxy>>("Proxy"));

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
