/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/coroutine/Task.h"

namespace rdd {

std::atomic<size_t> Task::count_(0);

ThreadTask::TaskMap ThreadTask::thread_tasks_;

}

