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

#include "raster/util/ReflectObject.h"
#include "raster/util/Logging.h"
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
