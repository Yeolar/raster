/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/HTTPHeaders.h"
#include <gtest/gtest.h>

using namespace rdd;

void checkEqual(const std::vector<std::string>& lhs,
                const std::vector<std::string>& rhs) {
  EXPECT_EQ(lhs.size(), rhs.size());
  for (size_t i = 0; i < lhs.size(); i++) {
    EXPECT_STREQ(lhs[i].c_str(), rhs[i].c_str());
  }
}

TEST(headers, all) {
  const char* data =
    "Foo: bar\r\n"
    " baz\r\n"
    "Asdf: qwer\r\n"
    "\tzxcv\r\n"
    "Foo: even\r\n"
    "     more\r\n"
    "     lines\r\n";

  HTTPHeaders headers(data);
  EXPECT_STREQ("qwer zxcv", headers.get("asdf").c_str());
  checkEqual({"qwer zxcv"}, headers.getList("asdf"));
  EXPECT_STREQ("bar baz,even more lines", headers.get("Foo").c_str());
  checkEqual({"bar baz", "even more lines"}, headers.getList("foo"));
}

