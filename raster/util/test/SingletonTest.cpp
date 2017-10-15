/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Singleton.h"
#include <gtest/gtest.h>

using namespace rdd;

class MyExpensiveService {
public:
  MyExpensiveService(int init = 0) : counter_(init) {}

  int counter() { return counter_++; }

private:
  int counter_;
};

namespace {

struct Tag1 {};
struct Tag2 {};
struct Tag3 {};
rdd::Singleton<MyExpensiveService> s_default;
rdd::Singleton<MyExpensiveService, Tag1> s1;
rdd::Singleton<MyExpensiveService, Tag2> s2;
rdd::Singleton<MyExpensiveService, Tag3> s_arg(
    []() { return new MyExpensiveService(10); });

}

TEST(Singleton, dflt) {
  MyExpensiveService* svc_default = s_default.get();
  EXPECT_EQ(0, svc_default->counter());
  EXPECT_EQ(1, svc_default->counter());
}

TEST(Singleton, tagged) {
  MyExpensiveService* svc1 = s1.get();
  MyExpensiveService* svc2 = s2.get();
  EXPECT_NE(svc1, svc2);
  EXPECT_EQ(0, svc1->counter());
  EXPECT_EQ(0, svc2->counter());
  MyExpensiveService* svc3 = Singleton<MyExpensiveService, Tag1>::get();
  MyExpensiveService* svc4 = Singleton<MyExpensiveService, Tag2>::get();
  EXPECT_EQ(1, svc3->counter());
  EXPECT_EQ(1, svc4->counter());
}

TEST(Singleton, arg) {
  MyExpensiveService* svc_arg = s_arg.get();
  EXPECT_EQ(10, svc_arg->counter());
  EXPECT_EQ(11, svc_arg->counter());
}
