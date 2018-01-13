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

#include "raster/protocol/http/HTTPHeaders.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(headers, all) {
  const char* data =
    "Foo: bar\r\n"
    " baz\r\n"
    "Asdf: qwer\r\n"
    "\tzxcv\r\n"
    "Foo: even\r\n"
    "     more\r\n"
    "     lines\r\n";

  HTTPHeaders headers;
  headers.parse(data);
  EXPECT_EQ(3, headers.size());
  EXPECT_EQ(1, headers.getNumberOfValues("asdf"));
  EXPECT_STREQ("qwer zxcv", headers.combine("asdf").c_str());
  EXPECT_STREQ("qwer zxcv", headers.getSingleOrEmpty("asdf").c_str());
  EXPECT_EQ(2, headers.getNumberOfValues("Foo"));
  EXPECT_STREQ("bar baz, even more lines", headers.combine("Foo").c_str());
  EXPECT_TRUE(headers.getSingleOrEmpty("Foo").empty());
}

