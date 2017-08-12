/*
 * Copyright (C) 2017, Yeolar
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
