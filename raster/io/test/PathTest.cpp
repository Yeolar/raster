/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/Path.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(Path, Construct) {
  EXPECT_STREQ("a/b/c", Path("a/b/c").c_str());
  EXPECT_STREQ("a/b/c", Path("x/../a/b/c").c_str());
  EXPECT_STREQ("c", Path("a/b/../../c").c_str());
  EXPECT_STREQ(".", Path("a/b/c/../../../").c_str());
  EXPECT_STREQ("../..", Path("a//../../../").c_str());
  EXPECT_STREQ("/a/b/c", Path("/a/b/c/").c_str());
  EXPECT_STREQ("/b/c", Path("/a/../b/c/").c_str());
  EXPECT_STREQ("/b/c", Path("/a/../../b/c/").c_str());
  EXPECT_STREQ("/", Path("/").c_str());
  EXPECT_STREQ(".", Path(".").c_str());
  EXPECT_STREQ("..", Path("..").c_str());
}

TEST(Path, Append) {
  EXPECT_STREQ("a/b/c", (Path("a/b") / "c").c_str());
  EXPECT_STREQ("a/c", (Path("a/b") / "../c").c_str());
  EXPECT_STREQ("c", (Path("a/b") / "../../c").c_str());
  EXPECT_STREQ("../c", (Path("a/b") / "../../../c").c_str());
  EXPECT_STREQ("/a/b/c", (Path("/a/b") / "c").c_str());
  EXPECT_STREQ("/c", (Path("a/b") / "/c").c_str());
  EXPECT_STREQ("/a/c", (Path("/a/b") / "../c").c_str());
  EXPECT_STREQ("/c", (Path("/a/b") / "../../c").c_str());
  EXPECT_STREQ("/c", (Path("/a/b") / "../../../c").c_str());
  EXPECT_STREQ("a/b/c", ("a/b" / Path("c")).c_str());
}

TEST(Path, Compare) {
  EXPECT_TRUE(Path().empty());
  EXPECT_TRUE(Path("").empty());
  EXPECT_FALSE(Path("a/..").empty());
  EXPECT_TRUE(Path("/a/b").isAbsolute());
  EXPECT_FALSE(Path("a/b").isAbsolute());
  EXPECT_EQ(Path("a/c"), Path("a/b/../c/"));
  EXPECT_GT(Path("b/c"), Path("a/c/"));
  EXPECT_LT(Path("a/c"), Path("b/c/"));
}

TEST(Path, Component) {
  EXPECT_STREQ("a/b", Path("a/b/c").parent().c_str());
  EXPECT_STREQ("/a/b", Path("/a/b/c").parent().c_str());
  EXPECT_STREQ("/", Path("/").parent().c_str());
  EXPECT_STREQ("..", Path(".").parent().c_str());
  EXPECT_STREQ("../..", Path("..").parent().c_str());

  EXPECT_STREQ("c", Path("a/b/c").name().c_str());
  EXPECT_STREQ("c", Path("/a/b/c").name().c_str());
  EXPECT_STREQ("", Path("/").name().c_str());
  EXPECT_STREQ(".", Path(".").name().c_str());
  EXPECT_STREQ("..", Path("..").name().c_str());

  EXPECT_STREQ("c", Path("a/b/c.d").base().c_str());
  EXPECT_STREQ("c.d", Path("/a/b/c.d.e").base().c_str());
  EXPECT_STREQ("a", Path("/a").base().c_str());
  EXPECT_STREQ(".a", Path(".a").base().c_str());
  EXPECT_STREQ(".", Path(".").base().c_str());
  EXPECT_STREQ("..", Path("..").base().c_str());

  EXPECT_STREQ(".d", Path("a/b/c.d").ext().c_str());
  EXPECT_STREQ(".e", Path("/a/b/c.d.e").ext().c_str());
  EXPECT_STREQ("", Path("/a").ext().c_str());
  EXPECT_STREQ("", Path(".a").ext().c_str());
  EXPECT_STREQ("", Path(".").ext().c_str());
  EXPECT_STREQ("", Path("..").ext().c_str());
}

