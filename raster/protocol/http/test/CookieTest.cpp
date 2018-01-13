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

#include "raster/protocol/http/Cookie.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(Cookie, all) {
  Cookie cookie;

  cookie.set("fig", "newton");
  cookie.set("sugar", "wafer");
  EXPECT_STREQ("Set-Cookie: fig=newton\r\n"
               "Set-Cookie: sugar=wafer", cookie.str().c_str());
  cookie.clear();

  cookie.set("rocky", "road");
  cookie.data["rocky"]->setAttr("path", "/cookie");
  EXPECT_STREQ("Set-Cookie: rocky=road; Path=/cookie", cookie.str().c_str());
  cookie.clear();

  cookie.load("chips=ahoy; vienna=finger");
  EXPECT_STREQ("Set-Cookie: chips=ahoy\r\n"
               "Set-Cookie: vienna=finger", cookie.str().c_str());
  cookie.clear();

  cookie.load("keebler=\"E=everybody; L=\\\"Loves\\\"; fudge=\\012;\";");
  EXPECT_STREQ("Set-Cookie: keebler=\"E=everybody; L=\\\"Loves\\\"; fudge=\\012;\"", cookie.str().c_str());
  cookie.clear();
}

