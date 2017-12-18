/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>
#include "raster/framework/Config.h"
#include "raster/framework/Signal.h"
#include "raster/net/Actor.h"
//#include "raster/protocol/http/AsyncClient.h"
#include "raster/protocol/http/AsyncServer.h"
#include "raster/util/Logging.h"
#include "raster/util/ReflectObject.h"
#include "raster/util/ScopeGuard.h"
#include "raster/util/Uuid.h"

static const char* VERSION = "1.0.0";

DEFINE_string(conf, "server.json", "Server config file");

namespace rdd {

class BaseHandler : public ReflectObject<RequestHandler, BaseHandler> {
public:
  BaseHandler() {
    RDDLOG(DEBUG) << "BaseHandler init";
  }

  virtual void onGet() {
    throw HTTPException(405);
  }
};

RDD_RF_REG(RequestHandler, BaseHandler);

}

using namespace rdd;

int main(int argc, char* argv[]) {
  google::SetVersionString(VERSION);
  google::SetUsageMessage("Usage : ./httpserver");
  google::ParseCommandLineFlags(&argc, &argv, true);

  setupIgnoreSignal(SIGPIPE);
  setupShutdownSignal(SIGINT);
  setupShutdownSignal(SIGTERM);

  HTTPAsyncServer* httpserver = new HTTPAsyncServer();
  httpserver->addRouter("BaseHandler", "/");

  Singleton<Actor>::get()->addService(
      "Http", std::shared_ptr<Service>(httpserver));

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
