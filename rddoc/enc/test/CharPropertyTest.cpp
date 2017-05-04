/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/enc/CharProperty.h"
#include <gtest/gtest.h>
#include "rddoc/util/Bits.h"
#include "rddoc/util/Macro.h"

using namespace rdd;

TEST(CharProperty, ucs2) {
  PeckerCharProperty char_prop;
  char_prop.compile("char.pecker.def", "char.ucs2.bin", "ucs2");
  char_prop.open("char.ucs2.bin");

  uint16_t hanzi_time[] = {
    0x5E74,0x6708,0x65E5,0x53F7,0x65F6,0x5206,0x79D2
  };
  for (size_t i = 0; i < NELEMS(hanzi_time); ++i) {
    EXPECT_TRUE(char_prop.isHanziTime(hanzi_time[i]));
  }
  uint16_t hanzi_digit[] = {
    0x3007,0x4E00,0x4E8C,0x4E09,0x56DB,0x4E94,0x516D,0x4E03,
    0x516B,0x4E5D,0x5341,0x767E,0x5343,0x4E07,0x4EBF
  };
  for (size_t i = 0; i < NELEMS(hanzi_digit); ++i) {
    EXPECT_TRUE(char_prop.isHanziDigit(hanzi_digit[i]));
  }
  uint16_t stop_punc[] = {
    0x0009,0x000A,0x000B,0x0021,0x003F,0x002C,0x003B,0x003D,
    0x005F,0x3002,0x2026,0xFF01,0xFF0C,0xFF0E,0xFF1A,0xFF1B,
    0xFF1F,0xFF3F
  };
  for (size_t i = 0; i < NELEMS(stop_punc); ++i) {
    EXPECT_TRUE(char_prop.isStopPunc(stop_punc[i]));
  }
}

TEST(CharProperty, gbk) {
  PeckerCharProperty char_prop;
  char_prop.compile("char.pecker.def", "char.gbk.bin", "gbk");
  char_prop.open("char.gbk.bin");

  char hanzi_time[] = "年月日号时分秒";
  for (size_t i = 0; i < strlen(hanzi_time); i += 2) {
    uint16_t c = Endian::big(*(uint16_t*)(hanzi_time + i));
    EXPECT_TRUE(char_prop.isHanziTime(c));
  }
  char hanzi_digit[] = "〇一二三四五六七八九十百千万亿兆几";
  for (size_t i = 0; i < strlen(hanzi_digit); i += 2) {
    uint16_t c = Endian::big(*(uint16_t*)(hanzi_digit + i));
    EXPECT_TRUE(char_prop.isHanziDigit(c));
  }
  char stop_punc[] = "。…！，．：；？＿";
  for (size_t i = 0; i < strlen(stop_punc); i += 2) {
    uint16_t c = Endian::big(*(uint16_t*)(stop_punc + i));
    EXPECT_TRUE(char_prop.isStopPunc(c));
  }
  char hanzi_other[] = "你妈叫你回家吃饭";
  for (size_t i = 0; i < strlen(hanzi_other); i += 2) {
    uint16_t c = Endian::big(*(uint16_t*)(hanzi_other + i));
    EXPECT_TRUE(char_prop.isHanziChar(c) && !(
            char_prop.isStopPunc(c) ||
            char_prop.isHanziDigit(c) ||
            char_prop.isHanziTime(c) ||
            char_prop.isStopPunc(c)));
  }
}
