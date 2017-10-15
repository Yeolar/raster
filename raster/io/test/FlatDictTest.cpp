/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/io/FlatDict.h"
#include "FlatDictTest_generated.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(FlatDict, Simple) {
  {
    FlatDict<int64_t> dict(100, "flatdicttest.dump");
    EXPECT_FALSE(!!dict.get(1));
    ::flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(
        fbs::CreatePerson(fbb,
                          fbb.CreateString("Yeolar"),
                          fbb.CreateString("Beijing")));
    ByteRange range(fbb.GetBufferPointer(), fbb.GetSize());
    dict.update(1, range);
    dict.update(2, range);
    EXPECT_EQ(dict.get(1).data, range);
    dict.erase(1);
    EXPECT_FALSE(!!dict.get(1));
    dict.sync();
  }
  {
    FlatDict<int64_t> dict(100, "flatdicttest.dump");
    dict.load("flatdicttest.dump");
    auto person = ::flatbuffers::GetRoot<fbs::Person>(dict.get(2).data.data());
    EXPECT_STREQ(person->name()->c_str(), "Yeolar");
    EXPECT_STREQ(person->city()->c_str(), "Beijing");
  }
}
