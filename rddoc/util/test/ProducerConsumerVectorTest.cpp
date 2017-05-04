/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/ProducerConsumerVector.h"
#include <gtest/gtest.h>

using namespace rdd;

void consumer(const std::vector<int>& v) {
  EXPECT_EQ(1, v[0]);
  EXPECT_EQ(2, v[1]);
  EXPECT_EQ(3, v[2]);
  EXPECT_EQ(4, v[3]);
}

TEST(ProducerConsumerVector, all) {
  ProducerConsumerVector<int> v;
  v.add(1);
  v.add(2);
  v.add(3);
  v.add(4);
  EXPECT_EQ(4, v.size());
  v.consume(consumer);
  EXPECT_EQ(0, v.size());
}
