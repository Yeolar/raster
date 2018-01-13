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

#include "raster/util/Arena.h"
#include "raster/util/Memory.h"
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>

using namespace rdd;

TEST(Arena, StlAllocator) {
  Arena arena(64);

  EXPECT_EQ(arena.totalSize(), sizeof(Arena));

  StlAllocator<Arena, size_t> alloc(&arena);
  std::vector<size_t, StlAllocator<Arena, size_t>> vec(alloc);

  for (size_t i = 0; i < 1000; i++) {
    vec.push_back(i);
  }

  for (size_t i = 0; i < 1000; i++) {
    EXPECT_EQ(i, vec[i]);
  }
}

TEST(ThreadArena, StlAllocator) {
  typedef std::unordered_map<
    int, int, std::hash<int>, std::equal_to<int>,
    StlAllocator<ThreadArena, std::pair<const int, int>>> Map;

  ThreadArena arena(64);

  StlAllocator<ThreadArena, std::pair<const int, int>> alloc(&arena);
  Map map(0, std::hash<int>(), std::equal_to<int>(), alloc);

  for (int i = 0; i < 1000; i++) {
    map[i] = i;
  }

  for (int i = 0; i < 1000; i++) {
    EXPECT_EQ(i, map[i]);
  }
}

