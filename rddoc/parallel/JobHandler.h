/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>
#include <vector>
#include "rddoc/parallel/Job.h"
#include "rddoc/util/ReflectObject.h"

namespace rdd {

class JobHandler {
public:
  JobHandler() {}
  virtual ~JobHandler() {}

  virtual void pipe(Job* job) = 0;
  virtual void map(Job* job) = 0;
  virtual void reduce(std::vector<Job*> jobs) = 0;

  int id;
  std::string name;
};

class JobHandlerManager {
public:
  JobHandlerManager() {}

  void addJobHandler(int id, const std::string& name) {
    if (handlers_.find(id) == handlers_.end()) {
      auto p = makeSharedReflectObject<JobHandler>(name);
      p->id = id;
      p->name = name;
      handlers_.emplace(id, p);
    }
  }

  JobHandler* getJobHandler(int id) {
    auto it = handlers_.find(id);
    return it != handlers_.end() ? it->second.get() : nullptr;
  }

private:
  std::map<int, std::shared_ptr<JobHandler>> handlers_;
};

}

