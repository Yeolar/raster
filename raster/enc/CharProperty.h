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

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "raster/util/Range.h"

namespace rdd {

class CharInfo {
 public:
  explicit CharInfo(size_t pos = none) {
    if (pos != none) {
      type_ |= (1u << pos);
    }
  }

  bool isKindOf(const CharInfo& ci) const {
    return (type_ & ci.type_) != 0;
  }

  CharInfo& operator+=(const CharInfo& other) {
    type_ += other.type_;
    return *this;
  }

  CharInfo& operator|=(const CharInfo& other) {
    type_ |= other.type_;
    return *this;
  }

  static constexpr size_t none = std::numeric_limits<size_t>::max();

 private:
  uint32_t type_{0};
};

inline CharInfo operator|(const CharInfo& lhs, const CharInfo& rhs) {
  CharInfo ci = lhs;
  ci |= rhs;
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

  size_t size() const;

  std::string name(size_t i) const;

  size_t id(StringPiece key) const;

  CharInfo getCharInfo(char16_t cp) const;

  CharInfo getCharInfo(StringPiece key) const;

 protected:
  std::vector<char> data_;
  std::vector<StringPiece> categories_;
  const CharInfo* map_{nullptr};
};

class PeckerCharProperty : public CharProperty {
 public:
  PeckerCharProperty() : CharProperty() {}

  bool open(const std::string& filename) override;

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

} // namespace rdd
