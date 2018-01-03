/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/Signal.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/protocol/http/AsyncServer.h"
#include "raster/util/Logging.h"
#include "raster/util/ReflectObject.h"
#include "raster/util/ScopeGuard.h"
#include "raster/util/Uuid.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace rdd;

class BaseHandler : public RequestHandler {
 public:
  BaseHandler() {
    RDDLOG(DEBUG) << "BaseHandler init";
  }

  ~BaseHandler() override {}

  void onGet() override {
    response
      .status(200)
      .body("Yes, got it.\r\n")
      .sendWithEOM();
  }
};

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./httpserver");
  google::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  auto httpserver = make_unique<HTTPAsyncServer>("HTTP");
  httpserver->addHandler<BaseHandler>("/");

  Singleton<HubAdaptor>::get()->addService(std::move(httpserver));

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
