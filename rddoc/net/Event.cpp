/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/net/Event.h"
#include "rddoc/net/EventTask.h"
#include "rddoc/net/Channel.h"

namespace rdd {

Event* Event::getCurrentEvent() {
  Task* task = ThreadTask::getCurrentThreadTask();
  return task ? ((EventTask*)task)->event() : nullptr;
}

Event::Event(const std::shared_ptr<Channel>& channel,
             const std::shared_ptr<Socket>& socket)
  : channel_(channel)
  , socket_(socket)
  , timeout_opt_(channel->timeoutOption()) {
  reset();
  RDD_EVLOG(V2, this) << "+";
}

Event::Event(Waker* waker) {
  type_ = WAKER;
  group_ = 0;
  action_ = NONE;
  waker_ = waker;
  task_ = nullptr;
}

Event::~Event() {
  user_ctx_.dispose();
  RDD_EVTLOG(V2, this) << "-";
}

void Event::reset() {
  restart();
  type_ = INIT;
  group_ = 0;
  action_ = NONE;
  waker_ = nullptr;
  task_ = nullptr;
  rbuf_ = IOBuf::create(Protocol::CHUNK_SIZE);
  wbuf_ = IOBuf::create(Protocol::CHUNK_SIZE);
  rlen_ = 0;
  wlen_ = 0;
}

void Event::restart() {
  if (!timestamps_.empty()) {
    RDD_EVTLOG(V2, this) << "restart";
    timestamps_.clear();
  }
  timestamps_.push_back(Timestamp(INIT));
}

void Event::record(Timestamp timestamp) {
  if (!timestamps_.empty()) {
    timestamp.stamp -= starttime();
  }
  timestamps_.push_back(timestamp);
}

std::string Event::label() const {
  return to<std::string>(roleLabel(), channel_->id());
}

const char* Event::typeName() const {
  static const char* TYPE_NAMES[] = {
    "INIT", "FAIL", "LISTEN", "TIMEOUT", "ERROR",
    "TOREAD", "READING", "READED",
    "TOWRITE", "WRITING", "WRITED",
    "NEXT", "CONNECT", "WAKER", "UNKNOWN",
  };
  int n = (int)NELEMS(TYPE_NAMES) - 1;
  int i = type_ >= 0 && type_ < n ? type_ : n;
  return TYPE_NAMES[i];
}

std::shared_ptr<Channel> Event::channel() const {
  return channel_;
}

std::shared_ptr<Processor> Event::processor(bool create) {
  if (create || !processor_) {
    if (!channel_->processorFactory()) {
      throw std::runtime_error("client channel has no processor");
    }
    processor_ = channel_->processorFactory()->create();
  }
  return processor_;
}

int Event::readData() {
  return channel_->protocol()->readData(this);
}

int Event::writeData() {
  return channel_->protocol()->writeData(this);
}

}

