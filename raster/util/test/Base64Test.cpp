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

#include "raster/util/Base64.h"
#include "raster/ssl/OpenSSLHash.h"
#include <gtest/gtest.h>

using namespace rdd;

static char kAsciiTable[] =
  "\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e\x0f"
  "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
  " !\"#$%&'()*+,-./"
  "0123456789:;<=>?"
  "@ABCDEFGHIJKLMNO"
  "PQRSTUVWXYZ[\\]^_"
  "`abcdefghijklmno"
  "pqrstuvwxyz{|}~\x7f"
  "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
  "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
  "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
  "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
  "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
  "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
  "\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
  "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff";

TEST(base64, all) {
  std::string str, enc, dec, hex;

  auto sha256 = [](const char* s) {
    std::array<uint8_t, 32> out;
    OpenSSLHash::sha256(range(out), ByteRange(StringPiece(s)));
    return std::string((char*)out.data(), out.size());
  };

  str = std::string(kAsciiTable, 256);
  base64Encode(str, enc);
  base64Decode(enc, dec);
  EXPECT_EQ(enc,
    "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v"
    "MDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5f"
    "YGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6P"
    "kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/"
    "wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v"
    "8PHy8/T19vf4+fr7/P3+/w==");
  EXPECT_EQ(dec, str);

  base64Encode(str, enc, Base64EncodeMode::URI);
  base64Decode(enc, dec, Base64EncodeMode::URI);
  EXPECT_EQ(enc,
    "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4v"
    "MDEyMzQ1Njc4OTo7PD0-P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5f"
    "YGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn-AgYKDhIWGh4iJiouMjY6P"
    "kJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq-wsbKztLW2t7i5uru8vb6_"
    "wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t_g4eLj5OXm5-jp6uvs7e7v"
    "8PHy8_T19vf4-fr7_P3-_w==");
  EXPECT_EQ(dec, str);

  str = sha256("");
  base64Encode(str, enc);
  base64Decode(enc, dec);
  hexlify(dec, hex);
  EXPECT_EQ(hex,
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

  str = sha256("1234567890");
  base64Encode(str, enc);
  base64Decode(enc, dec);
  hexlify(dec, hex);
  EXPECT_EQ(hex,
    "c775e7b757ede630cd0aa1113bd102661ab38829ca52a6422ab782862f268646");

  str = sha256("abcdefg");
  base64Encode(str, enc);
  base64Decode(enc, dec);
  hexlify(dec, hex);
  EXPECT_EQ(hex,
    "7d1a54127b222502f5b79b5fb0803061152a44f92b37e23c6527baf665d4da9a");
}
