/*
 * Copyright (C) 2017, Yeolar
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "rddoc/gen/Base.h"
#include "rddoc/gen/File.h"
#include "rddoc/io/File.h"
#include "rddoc/util/Range.h"
#include "rddoc/util/TestUtil.h"

using namespace rdd::gen;
using namespace rdd;
using std::string;
using std::vector;

TEST(FileGen, ByLine) {
  auto collect = eachTo<std::string>() | as<vector>();
  test::TemporaryFile file("ByLine");
  static const std::string lines(
      "Hello world\n"
      "This is the second line\n"
      "\n"
      "\n"
      "a few empty lines above\n"
      "incomplete last line");
  EXPECT_EQ(lines.size(), write(file.fd(), lines.data(), lines.size()));

  auto expected = from({lines}) | resplit('\n') | collect;
  auto found = byLine(file.path().c_str()) | collect;

  EXPECT_TRUE(expected == found);
}

class FileGenBufferedTest : public ::testing::TestWithParam<int> { };

TEST_P(FileGenBufferedTest, FileWriter) {
  size_t bufferSize = GetParam();
  test::TemporaryFile file("FileWriter");

  static const std::string lines(
      "Hello world\n"
      "This is the second line\n"
      "\n"
      "\n"
      "a few empty lines above\n");

  auto src = from({lines, lines, lines, lines, lines, lines, lines, lines});
  auto collect = eachTo<std::string>() | as<vector>();
  auto expected = src | resplit('\n') | collect;

  src | eachAs<StringPiece>() | toFile(File(file.fd()), bufferSize);
  auto found = byLine(file.path().c_str()) | collect;

  EXPECT_TRUE(expected == found);
}

TEST(FileGenBufferedTest, FileWriterSimple) {
  test::TemporaryFile file("FileWriter");
  auto toLine = [](int v) { return to<std::string>(v, '\n'); };

  auto squares = seq(1, 100) | map([](int x) { return x * x; });
  squares | map(toLine) | eachAs<StringPiece>() | toFile(File(file.fd()));
  EXPECT_EQ(squares | sum,
            byLine(File(file.path().c_str())) | eachTo<int>() | sum);
}

INSTANTIATE_TEST_CASE_P(
    DifferentBufferSizes,
    FileGenBufferedTest,
    ::testing::Values(0, 1, 2, 4, 8, 64, 4096));
