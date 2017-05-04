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
    space_       = getCharInfo("SPACE");
    alpha_lower_ = getCharInfo("ALPHA_LOWER");
    alpha_upper_ = getCharInfo("ALPHA_UPPER");
    ascii_digit_ = getCharInfo("ASCII_DIGIT");
    ascii_punc_  = getCharInfo("ASCII_PUNC");
    hanzi_char_  = getCharInfo("HANZI_CHAR");
    hanzi_digit_ = getCharInfo("HANZI_DIGIT");
    hanzi_time_  = getCharInfo("HANZI_TIME");
    full_width_  = getCharInfo("FULL_WIDTH");
    stop_punc_   = getCharInfo("STOP_PUNC");
    alpha_       = alpha_lower_ | alpha_upper_;
    alnum_       = alpha_ | ascii_digit_;
    return true;
  }

  bool isSpace     (char16_t c) const { return map_[c].isKindOf(space_); }
  bool isAlphaLower(char16_t c) const { return map_[c].isKindOf(alpha_lower_); }
  bool isAlphaUpper(char16_t c) const { return map_[c].isKindOf(alpha_upper_); }
  bool isAsciiDigit(char16_t c) const { return map_[c].isKindOf(ascii_digit_); }
  bool isAsciiPunc (char16_t c) const { return map_[c].isKindOf(ascii_punc_); }
  bool isHanziChar (char16_t c) const { return map_[c].isKindOf(hanzi_char_); }
  bool isHanziDigit(char16_t c) const { return map_[c].isKindOf(hanzi_digit_); }
  bool isHanziTime (char16_t c) const { return map_[c].isKindOf(hanzi_time_); }
  bool isFullWidth (char16_t c) const { return map_[c].isKindOf(full_width_); }
  bool isStopPunc  (char16_t c) const { return map_[c].isKindOf(stop_punc_); }
  bool isAlpha     (char16_t c) const { return map_[c].isKindOf(alpha_); }
  bool isAlnum     (char16_t c) const { return map_[c].isKindOf(alnum_); }

private:
  // single
  CharInfo space_;        // 空格
  CharInfo alpha_lower_;  // 小写英文字母
  CharInfo alpha_upper_;  // 大写英文字母
  CharInfo ascii_digit_;  // ascii数字
  CharInfo ascii_punc_;   // ascii标点符号
  CharInfo hanzi_char_;   // 中文汉字
  CharInfo hanzi_digit_;  // 中文数字
  CharInfo hanzi_time_;   // 中文时间
  CharInfo full_width_;   // 全角字符
  CharInfo stop_punc_;    // 分词切句标点
  // combined
  CharInfo alpha_;        // 英文字母
  CharInfo alnum_;        // 数字或英文字母
};

}

