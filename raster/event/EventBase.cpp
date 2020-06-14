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

#include "raster/event/EventBase.h"

DEFINE_uint64(event_lp_timeout, 600000000, "Long-polling timeout # of event.");

#define RASTER_EVENT_STR(state) #state

namespace {

static const char* stateStrings[] = {
  RASTER_EVENT_GEN(RASTER_EVENT_STR)
};

}

namespace raster {

void EventBase::setState(State state) {
  state_ = state;
  timestamps_.push_back(acc::StageTimestamp(state, acc::elapsed(startTime())));
}

const char* EventBase::stateName() const {
  return stateStrings[state_];
}

void EventBase::restart() {
  if (!timestamps_.empty()) {
    timestamps_.clear();
  }
  state_ = kInit;
  timestamps_.push_back(acc::StageTimestamp(kInit));
}

std::string EventBase::timestampStr() const {
  std::vector<std::string> v;
  for (auto& ts : timestamps_) {
    v.push_back(ts.str());
  }
  return acc::join("-", v);
}

} // namespace raster
