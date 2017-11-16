/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <vector>

#include "raster/util/Function.h"
#include "raster/util/json.h"

namespace rdd {

typedef std::pair<void(*)(const dynamic&, bool), std::string> ConfigTask;

std::string generateDefault();

void configLogging(const dynamic& j, bool reload);
void configProcess(const dynamic& j, bool reload);
void configActor(const dynamic& j, bool reload);
void configService(const dynamic& j, bool reload);
void configThreadPool(const dynamic& j, bool reload);
void configNetCopy(const dynamic& j, bool reload);
void configMonitor(const dynamic& j, bool reload);
void configDegrader(const dynamic& j, bool reload);
void configSampler(const dynamic& j, bool reload);
void configJobGraph(const dynamic& j, bool reload);

class ConfigManager {
public:
  ConfigManager() {}

  void setConfFile(const char* conf) {
    conf_ = conf;
  }

  void addTask(const ConfigTask& task) {
    tasks_.push_back(task);
  }

  void load();

private:
  const char* conf_;
  std::vector<ConfigTask> tasks_;
  bool inited_{false};
};

void config(const char* name, std::initializer_list<ConfigTask> confs);

} // namespace rdd
