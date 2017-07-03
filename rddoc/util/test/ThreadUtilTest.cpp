/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/ThreadUtil.h"
#include <thread>
#include <gtest/gtest.h>

using namespace rdd;

ThreadLocalPtr<int> tlp;

void* routine(void* ptr) {
  tlp.reset(new int(0));
  ++(*tlp.get());
  EXPECT_EQ(1, *tlp.get());
  return nullptr;
}

TEST(ThreadLocalPtr, all) {
  std::thread t(routine, nullptr);
  EXPECT_EQ(nullptr, tlp.get());
  t.join();
}
