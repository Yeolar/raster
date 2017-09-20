/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/event/Event.h"
#include "raster/io/event/EventExecutor.h"
#include "raster/net/Channel.h"

namespace rdd {

Event* Event::getCurrentEvent() {
  ExecutorPtr executor = getCurrentExecutor();
  return executor ? std::dynamic_pointer_cast<EventExecutor>(executor)->event() : nullptr;
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
  type_ = WAKER;
  group_ = 0;
  action_ = NONE;
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
  type_ = INIT;
  group_ = 0;
  action_ = NONE;
  waker_ = nullptr;
  executor_ = nullptr;
  rbuf_ = IOBuf::create(Protocol::CHUNK_SIZE);
  wbuf_ = IOBuf::create(Protocol::CHUNK_SIZE);
  rlen_ = 0;
  wlen_ = 0;
}

void Event::restart() {
  if (!timestamps_.empty()) {
    RDDLOG(V2) << *this << " restart";
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

} // namespace rdd
