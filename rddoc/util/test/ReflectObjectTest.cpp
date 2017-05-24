/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/ReflectObject.h"
#include "rddoc/util/Logging.h"
#include <gtest/gtest.h>

using namespace rdd;

class RObject {
public:
  virtual ~RObject() {}

  virtual int value() const { return 0; }
};

class A : public ReflectObject<RObject, A> {
public:
  virtual ~A() {}

  virtual int value() const { return 100; }
};

RDD_RF_REG(RObject, A);

TEST(ReflectObject, all) {
  RObject* p = makeReflectObject<RObject>("A");
  EXPECT_EQ(p->value(), 100);
  delete p;
}
