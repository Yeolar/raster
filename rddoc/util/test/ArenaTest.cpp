/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/Arena.h"
#include "rddoc/util/Memory.h"
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

