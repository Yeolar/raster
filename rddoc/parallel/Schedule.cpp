/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/parallel/Schedule.h"
#include "rddoc/util/DAG.h"
#include "rddoc/util/Singleton.h"

namespace rdd {

void scheduleNext(Event event, Job* job) {
  auto ids = Singleton<DAG<int>>::get()->next(job->id());
  for (auto i : ids) {
    auto j = event.getJob(i);
    j->appendPrevJob(job);
    if (j->finishPrevJob()) {
      sendJob(j);
    }
  }
}

void sendJob(Job* job) {
}

void runJob(Job* job) {
  auto* manager = Singleton<JobHandlerManager>::get();
  auto* handler = manager->getJobHandler(job->id());
  switch (job->type()) {
    case Job::PJOB:
      handler->pipe(job->prevJobs()[0]);
      break;
    case Job::MJOB:
      handler->map(job->prevJobs()[0]);
      break;
    case Job::RJOB:
      handler->reduce(job->prevJobs());
      break;
    default: break;
  }
}

}

