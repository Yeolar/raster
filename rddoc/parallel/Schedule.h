/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/parallel/Job.h"
#include "rddoc/parallel/JobHandler.h"

namespace rdd {

void scheduleNext(Event event, Job* job);

void sendJob(Job* job);

void runJob(Job* job);

}

