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

#include "raster/io/Cursor.h"
#include "raster/serializer/Serializer.h"
#include <map>
#include <string>
#include <vector>
#include <gtest/gtest.h>

namespace rdd {

/*
 * Outer base on acc::IOBuf, support string-like interface.
 */
class IOBufOuter {
public:
  IOBufOuter(acc::IOBuf* buf, uint64_t growth)
    : appender_(buf, growth) {}

  void append(const char* p, size_t n) {
    appender_.pushAtMost((uint8_t*)p, n);
  }

  void append(const std::string& s) {
    append(s.data(), s.size());
  }

private:
  io::Appender appender_;
};

template <class T, class Out>
inline typename std::enable_if<std::is_arithmetic<T>::value>::type
serialize(const T& value, Out& out) {
  out.append((char*)&value, sizeof(T));
}
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
    acc::IOBuf buf;
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
