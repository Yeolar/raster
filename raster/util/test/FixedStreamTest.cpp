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

#include "raster/util/FixedStream.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(FixedOstream, all) {
  char buf[256];
  FixedOstream stream(buf, sizeof(buf));

  EXPECT_EQ(buf, stream.output());
  EXPECT_EQ(buf, stream.output_ptr());
  EXPECT_EQ(buf + sizeof(buf), stream.output_end());
  EXPECT_EQ("", stream.str());

  stream << "abcdefg" << 1234567 << 'x';
  EXPECT_EQ("abcdefg1234567x", stream.str());

  stream.reset();

  EXPECT_EQ(buf, stream.output());
  EXPECT_EQ(buf, stream.output_ptr());
  EXPECT_EQ(buf + sizeof(buf), stream.output_end());
  EXPECT_EQ("", stream.str());
}
