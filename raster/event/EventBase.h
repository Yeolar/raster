/*
 * Copyright 2018 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <vector>

#include <accelerator/String.h>
#include <accelerator/Time.h>

#include "raster/Portability.h"
#include "raster/event/Timeout.h"

DECLARE_uint64(event_lp_timeout);

namespace raster {

#define RASTER_EVENT_GEN(x) \
  x(Init),                  \
  x(Connect),               \
  x(Listen),                \
  x(ToRead),                \
  x(Reading),               \
  x(Readed),                \
  x(ToWrite),               \
  x(Writing),               \
  x(Writed),                \
  x(Next),                  \
  x(Fail),                  \
  x(Timeout),               \
  x(Error),                 \
  x(Unknown)

#define RASTER_EVENT_ENUM(state) k##state

class EventBase {
 public:
  enum State {
    RASTER_EVENT_GEN(RASTER_EVENT_ENUM)
  };

  EventBase(const TimeoutOption& timeoutOpt)
    : timeoutOpt_(timeoutOpt) {}

  virtual ~EventBase() {}

  /*
   * Event state
   */

  State state() const;
  void setState(State state);

  const char* stateName() const;

  /*
   * Event timestamp and restart
   */

  void restart();

  uint64_t starttime() const;
  uint64_t cost() const;

  std::string timestampStr() const;

  /*
   * Event timeout
   */

  const TimeoutOption& timeoutOption() const;

  Timeout<EventBase> edeadline();
  Timeout<EventBase> cdeadline();
  Timeout<EventBase> rdeadline();
  Timeout<EventBase> wdeadline();

  bool isConnectTimeout() const;
  bool isReadTimeout() const;
  bool isWriteTimeout() const;

 public:
  virtual int fd() const = 0;
  virtual std::string str() const = 0;

 protected:
  State state_;
  std::vector<acc::StageTimestamp> timestamps_;
  TimeoutOption timeoutOpt_;
};

inline std::ostream& operator<<(std::ostream& os, const EventBase& event) {
  os << event.str();
  return os;
}

#undef RASTER_EVENT_ENUM

//////////////////////////////////////////////////////////////////////

inline EventBase::State EventBase::state() const {
  return state_;
}

inline uint64_t EventBase::starttime() const {
  return timestamps_.front().stamp;
}

inline uint64_t EventBase::cost() const {
  return acc::elapsed(starttime());
}

inline const TimeoutOption& EventBase::timeoutOption() const {
  return timeoutOpt_;
}

inline Timeout<EventBase> EventBase::edeadline() {
  return Timeout<EventBase>(this, starttime() + FLAGS_event_lp_timeout, true);
}

inline Timeout<EventBase> EventBase::cdeadline() {
  return Timeout<EventBase>(this, starttime() + timeoutOpt_.ctimeout);
}

inline Timeout<EventBase> EventBase::rdeadline() {
  return Timeout<EventBase>(this, starttime() + timeoutOpt_.rtimeout);
}

inline Timeout<EventBase> EventBase::wdeadline() {
  return Timeout<EventBase>(this, starttime() + timeoutOpt_.wtimeout);
}

inline bool EventBase::isConnectTimeout() const {
  return cost() > timeoutOpt_.ctimeout;
}

inline bool EventBase::isReadTimeout() const {
  return cost() > timeoutOpt_.rtimeout;
}

inline bool EventBase::isWriteTimeout() const {
  return cost() > timeoutOpt_.wtimeout;
}

} // namespace raster
