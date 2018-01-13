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

#include "raster/util/Hash.h"
#include <gtest/gtest.h>

using namespace rdd::hash;

// Not a full hasher since only handles one type
class TestHasher {
 public:
  static size_t hash(const std::pair<int, int>& p) {
    return p.first + p.second;
  }
};

template <typename T, typename... Ts>
size_t hash_combine_test(const T& t, const Ts&... ts) {
  return hash_combine_generic<TestHasher>(t, ts...);
}

TEST(Hash, hash_combine) {
  auto a = std::make_pair(1, 2);
  auto b = std::make_pair(3, 4);
  auto c = std::make_pair(1, 2);
  auto d = std::make_pair(2, 1);

  EXPECT_EQ(hash_combine(a), hash_combine(c));
  EXPECT_NE(hash_combine(b), hash_combine(c));
  EXPECT_NE(hash_combine(d), hash_combine(c));
  EXPECT_EQ(hash_combine(a, b), hash_combine(c, b));
  EXPECT_NE(hash_combine(a, b), hash_combine(b, a));

  // Test with custom hasher
  EXPECT_EQ(hash_combine_test(a), hash_combine_test(c));
  EXPECT_NE(hash_combine_test(b), hash_combine_test(c));
  EXPECT_EQ(hash_combine_test(d), hash_combine_test(c));
  EXPECT_EQ(hash_combine_test(a, b), hash_combine_test(c, b));
  EXPECT_NE(hash_combine_test(a, b), hash_combine_test(b, a));
  EXPECT_EQ(hash_combine_test(a, b), hash_combine_test(d, b));
}
