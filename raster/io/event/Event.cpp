/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/event/Event.h"
#include "raster/io/event/EventExecutor.h"
#include "raster/net/Channel.h"

#define RDD_IO_EVENT_STR(role) #role

namespace {
  static const char* stateStrings[] = {
    RDD_IO_EVENT_GEN(RDD_IO_EVENT_STR)
  };
}

namespace rdd {

Event* Event::getCurrent() {
  ExecutorPtr executor = getCurrentExecutor();
  return executor ?
    std::dynamic_pointer_cast<EventExecutor>(executor)->event() : nullptr;
}

std::atomic<uint64_t> Event::globalSeqid_(1);

Event::Event(const std::shared_ptr<Channel>& channel,
             const std::shared_ptr<Socket>& socket)
  : channel_(channel)
  , socket_(socket)
  , timeoutOpt_(channel->timeoutOption()) {
  reset();
  RDDLOG(V2) << *this << " +";
}

Event::Event(Waker* waker) {
  seqid_ = 0;
  state_ = kWaker;
  group_ = 0;
  forward_ = false;
  waker_ = waker;
  executor_ = nullptr;
}

Event::~Event() {
  userCtx_.dispose();
  RDDLOG(V2) << *this << " -";
}

void Event::reset() {
  restart();

  seqid_ = globalSeqid_.fetch_add(1);
  state_ = kInit;
  group_ = 0;
  forward_ = false;
  waker_ = nullptr;
  executor_ = nullptr;

  if (transport_) {
    transport_->reset();
  } else {
    if (channel_->transportFactory()) {
      transport_ = std::move(channel_->transportFactory()->create());
    }
  }
}

void Event::restart() {
  if (!timestamps_.empty()) {
    RDDLOG(V2) << *this << " restart";
    timestamps_.clear();
  }
  timestamps_.push_back(Timestamp(kInit));
}

void Event::record(Timestamp timestamp) {
  if (!timestamps_.empty()) {
    timestamp.stamp -= starttime();
  }
  timestamps_.push_back(timestamp);
}

std::string Event::label() const {
  return to<std::string>(descriptor()->role()[0], channel_->id());
}

const char* Event::stateName() const {
  if (state_ < kInit || state_ >= kUnknown) {
    return stateStrings[kUnknown];
  } else {
    return stateStrings[state_];
  }
}

std::shared_ptr<Channel> Event::channel() const {
  return channel_;
}

std::unique_ptr<Processor> Event::processor() {
  if (!channel_->processorFactory()) {
    throw std::runtime_error("client channel has no processor");
  }
  return channel_->processorFactory()->create(this);
}

} // namespace rdd
