/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/util/json.h"

namespace rdd {

typedef std::pair<void(*)(const dynamic&), std::string> ConfigTask;

std::string generateDefault();

void configLogging(const dynamic& j);
void configActor(const dynamic& j);
void configService(const dynamic& j);
void configThreadPool(const dynamic& j);
void configNetCopy(const dynamic& j);
void configMonitor(const dynamic& j);
void configJobGraph(const dynamic& j);
void configDegrader(const dynamic& j);
void configSampler(const dynamic& j);

void config(const char* name, std::initializer_list<ConfigTask> confs);

} // namespace rdd
