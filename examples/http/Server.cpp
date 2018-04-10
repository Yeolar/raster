/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/framework/Config.h"
#include "raster/framework/Signal.h"
#include "raster/framework/HubAdaptor.h"
#include "raster/protocol/http/AsyncServer.h"
#include "accelerator/Logging.h"
#include "accelerator/Portability.h"
#include "accelerator/ReflectObject.h"
#include "accelerator/ScopeGuard.h"
#include "accelerator/Uuid.h"

static const char* VERSION = "1.1.0";

DEFINE_string(conf, "server.json", "Server config file");

using namespace rdd;

class BaseHandler : public RequestHandler {
 public:
  BaseHandler() {
    ACCLOG(DEBUG) << "BaseHandler init";
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
  gflags::SetVersionString(VERSION);
  gflags::SetUsageMessage("Usage : ./httpserver");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  auto httpserver = acc::make_unique<HTTPAsyncServer>("HTTP");
  httpserver->addHandler<BaseHandler>("/");

  acc::Singleton<HubAdaptor>::get()->addService(std::move(httpserver));

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
