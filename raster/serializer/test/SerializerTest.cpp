/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/serializer/Serializer.h"
#include <map>
#include <string>
#include <vector>
#include <gtest/gtest.h>

namespace rdd {

struct BasicTypeObject {
  bool b{false};
  int i{0};
  float f{0.0};
  std::string s;
};

RDD_SERIALIZER(BasicTypeObject, b, i, f, s)

struct NestedObject {
  BasicTypeObject o;
  std::vector<BasicTypeObject> a;
  std::map<std::string, BasicTypeObject> m;
};

RDD_SERIALIZER(NestedObject, o, a, m)

}

using namespace rdd;

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
    object.o.b = true;
    object.o.i = 10;
    object.o.f = 0.1;
    object.o.s = "abc";
    object.a.push_back(object.o);
    object.m.emplace("key", object.o);
    serialize(object, out);
    unserialize(range(out), 0, outObject);
    EXPECT_EQ(outObject.o.b, true);
    EXPECT_EQ(outObject.o.i, 10);
    EXPECT_FLOAT_EQ(outObject.o.f, 0.1);
    EXPECT_STREQ(outObject.o.s.c_str(), "abc");
    EXPECT_EQ(outObject.a.size(), 1);
    EXPECT_EQ(outObject.m.size(), 1);
  }
}
