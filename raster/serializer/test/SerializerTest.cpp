/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/serializer/Serializer.h"
#include <map>
#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace rdd;
using namespace rdd::bsp;

struct BasicTypeObject {
  bool b{false};
  int i{0};
  float f{0.0};
  std::string s;
};

RDD_BSP_SERIALIZER(BasicTypeObject, b, i, f, s)

struct NestedObject {
  BasicTypeObject object;
  std::vector<BasicTypeObject> list;
  std::map<std::string, BasicTypeObject> map;
};

RDD_BSP_SERIALIZER(NestedObject, object, list, map);

TEST(bsp_serialize, basic) {
  {
    std::string out;
    std::vector<int> a = { 1, 2, 3, 4, 5 };
    std::vector<int> b;
    serialize(a, out);
    unserialize(range(out), 0, b);
    EXPECT_EQ(b.size(), 5);
    for (size_t i = 0; i < b.size(); i++) {
      EXPECT_EQ(b[i], i + 1);
    }
  }
  {
    IOBuf buf;
    IOBufOuter out(&buf, 1024);
    std::vector<int> a = { 1, 2, 3, 4, 5 };
    std::vector<int> b;
    serialize(a, out);
    unserialize(buf.coalesce(), 0, b);
    EXPECT_EQ(b.size(), 5);
    for (size_t i = 0; i < b.size(); i++) {
      EXPECT_EQ(b[i], i + 1);
    }
  }
}

TEST(bsp_serialize, structure) {
  {
    std::string out;
    BasicTypeObject object, outObject;
    object.b = true;
    object.i = 10;
    object.f = 0.1;
    object.s = "abc";
    serialize(object, out);
    unserialize(range(out), 0, outObject);
    EXPECT_EQ(outObject.b, true);
    EXPECT_EQ(outObject.i, 10);
    EXPECT_FLOAT_EQ(outObject.f, 0.1);
    EXPECT_STREQ(outObject.s.c_str(), "abc");
  }
  {
    std::string out;
    NestedObject object, outObject;
    object.object.b = true;
    object.object.i = 10;
    object.object.f = 0.1;
    object.object.s = "abc";
    object.list.push_back(object.object);
    object.map.emplace("key", object.object);
    serialize(object, out);
    unserialize(range(out), 0, outObject);
    EXPECT_EQ(outObject.object.b, true);
    EXPECT_EQ(outObject.object.i, 10);
    EXPECT_FLOAT_EQ(outObject.object.f, 0.1);
    EXPECT_STREQ(outObject.object.s.c_str(), "abc");
    EXPECT_EQ(outObject.list.size(), 1);
    EXPECT_EQ(outObject.map.size(), 1);
  }
}
