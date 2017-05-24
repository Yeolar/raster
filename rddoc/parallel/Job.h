/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stddef.h>
#include <memory>
#include <vector>

namespace rdd {

class Job {
public:
  enum Type {
    PJOB,   // pipe
    MJOB,   // map
    RJOB,   // reduce
  };

  Job(Type type, int id, size_t prev_count)
    : type_(type), id_(id), waits_(prev_count) {}

  virtual ~Job() {}

  Type type() const { return type_; }
  int id() const { return id_; }

  bool finishPrevJob() {
    return --waits_ == 0;
  }

  void appendPrevJob(Job* job) {
    prev_jobs_.push_back(job);
  }

  std::vector<Job*> prevJobs() const { return prev_jobs_; }

private:
  Type type_;
  int id_;
  size_t waits_;
  std::vector<Job*> prev_jobs_;
};

}

