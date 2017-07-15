/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

namespace rdd {

class CharInfo {
public:
  CharInfo() {}
  CharInfo(size_t pos) { set(pos); }

  void set(size_t pos) {
    type |= ((uint32_t)1 << pos);
  }

  void reset(size_t pos) {
    type &= ~((uint32_t)1 << pos);
  }

  bool isKindOf(const CharInfo ci) const {
    return (type & ci.type) != 0;
  }

  uint32_t type{0};
};

inline CharInfo operator|(const CharInfo lhs, const CharInfo rhs) {
  CharInfo ci;
  ci.type = lhs.type | rhs.type;
  return ci;
}

class CharProperty {
public:
  // available charset: ucs2, gbk
  static bool compile(const std::string& cfile,
                      const std::string& ofile,
                      const std::string& charset = "ucs2");

  CharProperty() {}

  virtual bool open(const std::string& filename);

  size_t size() const { return categories_.size(); }

  const char* name(size_t i) const { return categories_[i]; }

  size_t id(const char* key) const {
    for (size_t i = 0; i < categories_.size(); ++i) {
      if (strcmp(key, categories_[i]) == 0)
        return i;
    }
    return SIZE_MAX;
  }

  CharInfo getCharInfo(const char* key) const {
    size_t i = id(key);
    return i != SIZE_MAX ? CharInfo(i) : CharInfo();
  }

  CharInfo getCharInfo(char16_t cp) const { return map_[cp]; }

protected:
  std::string data_;
  std::vector<const char*> categories_;
  const CharInfo* map_{nullptr};
};

class PeckerCharProperty : public CharProperty {
public:
  PeckerCharProperty() : CharProperty() {}

  virtual bool open(const std::string& filename) {
    if (!CharProperty::open(filename)) {
      return false;
    }
    space_      = getCharInfo("SPACE");
    alphaLower_ = getCharInfo("ALPHA_LOWER");
    alphaUpper_ = getCharInfo("ALPHA_UPPER");
    asciiDigit_ = getCharInfo("ASCII_DIGIT");
    asciiPunc_  = getCharInfo("ASCII_PUNC");
    hanziChar_  = getCharInfo("HANZI_CHAR");
    hanziDigit_ = getCharInfo("HANZI_DIGIT");
    hanziTime_  = getCharInfo("HANZI_TIME");
    fullWidth_  = getCharInfo("FULL_WIDTH");
    stopPunc_   = getCharInfo("STOP_PUNC");
    alpha_      = alphaLower_ | alphaUpper_;
    alnum_      = alpha_ | asciiDigit_;
    return true;
  }

  bool isSpace     (char16_t c) const { return map_[c].isKindOf(space_); }
  bool isAlphaLower(char16_t c) const { return map_[c].isKindOf(alphaLower_); }
  bool isAlphaUpper(char16_t c) const { return map_[c].isKindOf(alphaUpper_); }
  bool isAsciiDigit(char16_t c) const { return map_[c].isKindOf(asciiDigit_); }
  bool isAsciiPunc (char16_t c) const { return map_[c].isKindOf(asciiPunc_); }
  bool isHanziChar (char16_t c) const { return map_[c].isKindOf(hanziChar_); }
  bool isHanziDigit(char16_t c) const { return map_[c].isKindOf(hanziDigit_); }
  bool isHanziTime (char16_t c) const { return map_[c].isKindOf(hanziTime_); }
  bool isFullWidth (char16_t c) const { return map_[c].isKindOf(fullWidth_); }
  bool isStopPunc  (char16_t c) const { return map_[c].isKindOf(stopPunc_); }
  bool isAlpha     (char16_t c) const { return map_[c].isKindOf(alpha_); }
  bool isAlnum     (char16_t c) const { return map_[c].isKindOf(alnum_); }

private:
  // single
  CharInfo space_;        // 空格
  CharInfo alphaLower_;   // 小写英文字母
  CharInfo alphaUpper_;   // 大写英文字母
  CharInfo asciiDigit_;   // ascii数字
  CharInfo asciiPunc_;    // ascii标点符号
  CharInfo hanziChar_;    // 中文汉字
  CharInfo hanziDigit_;   // 中文数字
  CharInfo hanziTime_;    // 中文时间
  CharInfo fullWidth_;    // 全角字符
  CharInfo stopPunc_;     // 分词切句标点
  // combined
  CharInfo alpha_;        // 英文字母
  CharInfo alnum_;        // 数字或英文字母
};

}

