/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/framework/AutoTask.h"
#include <gtest/gtest.h>

using namespace rdd;

class Task : public AutoTask {
public:
  Task() : times_(0) {}

  void update() { times_++; }
  size_t times() const { return times_; }

private:
  size_t times_;
};

TEST(AutoTaskManager, runTasks) {
  AutoTaskManager atm;
  Task a, b;
  atm.addTask(&a, 10000);
  atm.addTask(&b, 1000);
  EXPECT_EQ(0, a.times());
  EXPECT_EQ(0, b.times());
  atm.initStamp();
  atm.runTasks();
  EXPECT_EQ(1, a.times());
  EXPECT_EQ(1, b.times());
  usleep(1000);
  atm.runTasks();
  EXPECT_EQ(1, a.times());
  EXPECT_EQ(2, b.times());
  usleep(4000);
  atm.runTasks();
  EXPECT_EQ(1, a.times());
  EXPECT_EQ(3, b.times());
  usleep(5000);
  atm.runTasks();
  EXPECT_EQ(2, a.times());
  EXPECT_EQ(4, b.times());
}
