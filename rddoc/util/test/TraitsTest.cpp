/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/Traits.h"
#include <gtest/gtest.h>

using namespace rdd;

class A {
public:
  void test() const {}
};

RDD_CREATE_HAS_MEMBER_FN_TRAITS(has_test_traits, test);

TEST(Traits, is_xxx) {
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
