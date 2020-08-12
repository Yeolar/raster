/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <vector>

#include <accelerator/json.h>

#include "raster/concurrency/Executor.h"

namespace raster {

typedef std::pair<void(*)(const acc::dynamic&, bool), std::string> ConfigTask;

std::string generateDefault();

void configLogging(const acc::dynamic& j, bool reload);
void configProcess(const acc::dynamic& j, bool reload);
void configService(const acc::dynamic& j, bool reload);
void configThreadPool(const acc::dynamic& j, bool reload);
void configNet(const acc::dynamic& j, bool reload);
void configMonitor(const acc::dynamic& j, bool reload);
void configDegrader(const acc::dynamic& j, bool reload);
void configSampler(const acc::dynamic& j, bool reload);
void configJobGraph(const acc::dynamic& j, bool reload);

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

} // namespace raster
