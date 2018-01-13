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

#include <string>
#include "raster/parallel/DAG.h"
#include "raster/util/Logging.h"
#include "raster/util/String.h"
#include <gtest/gtest.h>

using namespace rdd;

class IntExecutor : public Executor {
public:
  IntExecutor(int i, std::string* out) : key_(i), out_(out) {}

  void run() {
    stringAppendf(out_, "%d ", key_);
  }

private:
  int key_;
  std::string* out_;
};

TEST(DAG, all) {
  DAG dag;
  std::string out;

  dag.add(std::make_shared<IntExecutor>(0, &out));
  dag.add(std::make_shared<IntExecutor>(1, &out));
  dag.add(std::make_shared<IntExecutor>(2, &out));
  dag.add(std::make_shared<IntExecutor>(3, &out));
  dag.add(std::make_shared<IntExecutor>(4, &out));
  /*       2 -+
   * 0 -+      \
   *     \      \
   * 1 --- 3 ---- 4
   */
  dag.dependency(0, 3);
  dag.dependency(1, 3);
  dag.dependency(2, 4);
  dag.dependency(3, 4);

  dag.schedule(dag.go(std::make_shared<IntExecutor>(5, &out)));
  EXPECT_STREQ("0 1 3 2 4 5 ", out.c_str());
}
