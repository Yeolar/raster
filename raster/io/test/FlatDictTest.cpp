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
    EXPECT_EQ(dict.size(), 2);
  }
  {
    FlatDict<int64_t> dict(100, "flatdicttest.dump");
    dict.load("flatdicttest.dump");
    auto person = ::flatbuffers::GetRoot<fbs::Person>(dict.get(2).data.data());
    EXPECT_STREQ(person->name()->c_str(), "Yeolar");
    EXPECT_STREQ(person->city()->c_str(), "Beijing");
    EXPECT_EQ(dict.size(), 1);
  }
}

TEST(FlatDict, Thread) {
  FlatDict<uint64_t> dict(10000000);
  ::flatbuffers::FlatBufferBuilder fbb;
  fbb.Finish(
      fbs::CreatePerson(fbb,
                        fbb.CreateString("Yeolar"),
                        fbb.CreateString("Beijing")));
  ByteRange range(fbb.GetBufferPointer(), fbb.GetSize());

  std::atomic<bool> go;
  std::vector<std::thread> threads;

  while (threads.size() < 8) {
    threads.emplace_back([&]() {
      while (!go) {
        std::this_thread::yield();
      }
      for (int i = 0; i < 1000000; i++) {
        dict.update(Random::rand64(), range);
      }
    });
  }
  go = true;

  for (auto& t : threads) {
    t.join();
  }
}

TEST(FlatDict, Conflict) {
  FlatDict<uint64_t> dict(10000);
  ::flatbuffers::FlatBufferBuilder fbb1;
  fbb1.Finish(
      fbs::CreatePerson(fbb1,
                        fbb1.CreateString("Yeolar"),
                        fbb1.CreateString("Beijing")));
  ByteRange range1(fbb1.GetBufferPointer(), fbb1.GetSize());
  ::flatbuffers::FlatBufferBuilder fbb2;
  fbb2.Finish(
      fbs::CreatePerson(fbb2,
                        fbb2.CreateString("Yeolar"),
                        fbb2.CreateString("Shanghai")));
  ByteRange range2(fbb2.GetBufferPointer(), fbb2.GetSize());
  for (int i = 0; i < 10000; i++) {
    dict.update(i, range1);
    dict.update(i, range2);
    auto person = ::flatbuffers::GetRoot<fbs::Person>(dict.get(i).data.data());
    EXPECT_STREQ(person->name()->c_str(), "Yeolar");
    EXPECT_STREQ(person->city()->c_str(), "Shanghai");
  }
  EXPECT_EQ(dict.size(), 10000);
}
