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

#include "raster/util/Traits.h"
#include <gtest/gtest.h>

using namespace rdd;

class A {
public:
  void test() const {}
};

RDD_CREATE_HAS_MEMBER_FN_TRAITS(has_test_traits, test);

TEST(Traits, is_xxx) {
  EXPECT_TRUE(IsRelocatable<int>::value);
  EXPECT_TRUE(IsRelocatable<bool>::value);
  EXPECT_TRUE(IsRelocatable<double>::value);
  EXPECT_TRUE(IsRelocatable<void*>::value);

  EXPECT_TRUE (IsTriviallyCopyable<int>::value);
  EXPECT_FALSE(IsTriviallyCopyable<std::vector<int>>::value);
  EXPECT_TRUE (IsZeroInitializable<int>::value);
  EXPECT_FALSE(IsZeroInitializable<std::vector<int>>::value);
}

TEST(Traits, has_member_fn_traits) {
  bool has_fn;

  has_fn = has_test_traits<A, void() const>::value;
  EXPECT_TRUE(has_fn);
  has_fn = has_test_traits<A, int() const>::value;
  EXPECT_FALSE(has_fn);
}
