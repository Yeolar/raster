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

#include "raster/util/Logging.h"
#include <gtest/gtest.h>

using namespace rdd;
using namespace rdd::logging;

TEST(RDDLog, log) {
  RDDLOG(DEBUG) << "debug log default output to stderr";
  RDDLOG(INFO) << "info log default output to stderr";
  RDDLOG(WARN) << "warning log default output to stderr";
  RDDLOG(ERROR) << "error log default output to stderr";
}

TEST(RDDLog, plog) {
  int saved_errno = errno;
  errno = EAGAIN;
  RDDPLOG(WARN) << "EAGAIN warning log";
  errno = saved_errno;
}

TEST(RDDLog, setLevel) {
  Singleton<RDDLogger>::get()->setLevel(logging::LOG_INFO);
  RDDLOG(DEBUG) << "debug log SHOULD NOT BE SEEN";
  RDDLOG(INFO) << "info log available";
  Singleton<RDDLogger>::get()->setLevel(logging::LOG_DEBUG);
  RDDLOG(DEBUG) << "debug log available";
  RDDLOG(INFO) << "info log available";
}

TEST(RDDLog, async) {
  Singleton<RDDLogger>::get()->setLogFile("log.txt");
  RDDLOG(INFO) << "async info log to file";
  Singleton<RDDLogger>::get()->setLogFile("nondir/log.txt");
}

TEST(RDDLog, raw) {
  RDDRLOG(INFO) << "raw log";
}

TEST(RDDLog, cost) {
  RDDCOST_SCOPE(INFO, 1000) {
    usleep(100);
  };
  RDDCOST_SCOPE(INFO, 1000) {
    usleep(10000);
  };
}
