/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/Sha.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(sha256, all) {
  EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            sha256Hex(std::string("")));
  EXPECT_EQ("c775e7b757ede630cd0aa1113bd102661ab38829ca52a6422ab782862f268646",
            sha256Hex(std::string("1234567890")));
  EXPECT_EQ("7d1a54127b222502f5b79b5fb0803061152a44f92b37e23c6527baf665d4da9a",
            sha256Hex(std::string("abcdefg")));
  EXPECT_EQ("88a00f46836cd629d0b79de98532afde3aead79a5c53e4848102f433046d0106",
            sha256Hex(std::string("1234567890"), std::string("abcdefg")));
}
