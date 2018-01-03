/*
 * Copyright (C) 2017, Yeolar
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

