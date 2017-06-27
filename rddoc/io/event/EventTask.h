/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "rddoc/coroutine/Task.h"
#include "rddoc/io/event/Event.h"

namespace rdd {

class EventTask : public Task {
public:
  explicit EventTask(Event* event, int stack_size, int pid)
    : Task(stack_size, pid), event_(event) {}

  virtual ~EventTask() {}

  virtual int type() const { return NET; }
  virtual void close();
  virtual bool isConnectionClosed();

  Event* event() const { return event_; }

private:
  Event* event_;
};

}
