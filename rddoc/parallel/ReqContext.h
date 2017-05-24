/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <memory>
#include "rddoc/parallel/Job.h"

namespace rdd {

class ReqContext {
public:
  ReqContext() {}

  void initJobs() {
    ;
  }

  Job* getJob(int id) const {
    auto it = jobs_.find(id);
    return it != jobs_.end() ? it->second.get() : nullptr;
  }

private:
  std::map<int, std::shared_ptr<Job>> jobs_;
};

}

