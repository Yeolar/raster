/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/EventTask.h"

namespace rdd {

bool EventTask::isConnectionClosed() {
  char p[8];
  int r = event_->socket()->recv(p, sizeof(p));
  if (r == 0) {
    RDDLOG(V1) << "connection already closed";
    return true;
  }
  return false;
}

void EventTask::close() {
  if (event_) {
    delete event_;
  }
  event_ = nullptr;
}

}
