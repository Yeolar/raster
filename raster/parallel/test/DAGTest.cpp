/*
 * Copyright (C) 2017, Yeolar
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
