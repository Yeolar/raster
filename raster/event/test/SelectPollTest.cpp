/*
 * Copyright 2020 Yeolar
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

#include <gtest/gtest.h>
#include <accelerator/Waker.h>
#include "raster/event/SelectPoll.h"
#include "raster/event/test/EventBaseTest.h"

using namespace raster;

TEST(SelectPoll, all) {
  SelectPoll poll(64);
  acc::Waker waker;

  poll.event(waker.fd()) = new Event(waker.fd());
  poll.add(waker.fd(), EventBase::kRead);
  int n = 0;

  waker.wake();
  n = poll.wait(1);
  EXPECT_EQ(1, n);
  EXPECT_EQ(waker.fd(), poll.firedFds()[0].fd);

  waker.consume();
  n = poll.wait(1);
  EXPECT_EQ(0, n);
}
