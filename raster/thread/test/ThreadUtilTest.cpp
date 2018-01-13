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

#include "raster/util/ThreadUtil.h"
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
