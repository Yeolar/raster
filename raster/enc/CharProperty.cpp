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

#include "raster/enc/CharProperty.h"

#include <map>

#include "raster/io/FileUtil.h"
#include "raster/util/Logging.h"
#include "raster/util/String.h"

namespace rdd {

static constexpr size_t kMaxUnicodeLength = 65536;
static constexpr size_t kMaxCategoryLength = 32;

uint16_t gUnicodeToGBKTable[kMaxUnicodeLength] = {
#include "un2gbkmap.h"
};

/*
 * format:
 *  | n (uint32) | category ... (char[32] * n) | charinfo ... (charinfo * n) |
 */
bool CharProperty::open(const std::string& filename) {
  if (!readFile(filename.c_str(), data_)) {
    RDDLOG(ERROR) << "read char property file " << filename << " failed";
    return false;
  }
  uint32_t n = *((uint32_t*)data_.data());
  uint32_t offset = sizeof(uint32_t);
  if (data_.size() != (offset + kMaxCategoryLength * n
                       + sizeof(uint32_t) * 0xffff)) {
    RDDLOG(ERROR) << "illegal size of char property file " << filename;
    return false;
  }
  categories_.clear();
  for (uint32_t i = 0; i < n; ++i) {
    categories_.push_back(&data_[offset]);
    offset += kMaxCategoryLength;
  }
  map_ = (const CharInfo*) &data_[offset];
  return true;
}

size_t CharProperty::size() const {
  return categories_.size();
}

std::string CharProperty::name(size_t i) const {
  return categories_[i].str();
}

size_t CharProperty::id(StringPiece key) const {
  for (size_t i = 0; i < categories_.size(); ++i) {
    if (key == categories_[i]) {
      return i;
    }
  }
  return std::numeric_limits<size_t>::max();
}

CharInfo CharProperty::getCharInfo(char16_t cp) const {
  return map_[cp];
}

CharInfo CharProperty::getCharInfo(StringPiece key) const {
  return CharInfo(id(key));
}

char16_t readHex(StringPiece s) {
  auto unhex = [](char c) -> int {
    return c >= '0' && c <= '9' ? c - '0' :
           c >= 'a' && c <= 'f' ? c - 'a' + 10 :
           c >= 'A' && c <= 'F' ? c - 'A' + 10 :
           -1;
  };
  // 0x0000 - 0xffff
  char16_t r = unhex(s[2]) * 0x1000
             + unhex(s[3]) * 0x100
             + unhex(s[4]) * 0x10
             + unhex(s[5]);
  return r;
}

struct CharRange {
  char16_t l;                 // code point start
  char16_t h;                 // code point end
  std::vector<StringPiece> c; // code point categories
};

CharRange parseCharRangeFromLine(StringPiece line) {
  CharRange range;
  std::vector<StringPiece> cols, v;
  split(' ', line, cols, true);
  split("..", cols[0], v);
  range.l = readHex(v[0]);
  range.h = v.size() > 1 ? readHex(v[1]) : range.l;
  if (range.l >= 0xffff || range.h >= 0xffff || range.l > range.h) {
    RDDLOG(ERROR) << "code point range error: " << cols[0];
    return range;
  }
  for (size_t i = 1; i < cols.size(); ++i) {
    range.c.push_back(cols[i]);
  }
  return range;
}

CharInfo encode(const std::vector<StringPiece>& c,
                const std::map<StringPiece, CharInfo>& m) {
  CharInfo base;
  if (c.empty()) {
    RDDLOG(ERROR) << "no category";
    return base;
  }
  for (auto& i : c) {
    auto it = m.find(i);
    if (it == m.end()) {
      RDDLOG(ERROR) << "category[" << i << "] is undefined";
      return base;
    }
    base += it->second;
  }
  return base;
}

bool CharProperty::compile(const std::string& cfile,
                           const std::string& ofile,
                           const std::string& charset) {
  std::string conf;
  if (!readFile(cfile.c_str(), conf)) {
    RDDLOG(ERROR) << "read conf file " << cfile << " failed";
    return false;
  }
  std::vector<CharRange> ranges;
  std::map<StringPiece, CharInfo> cmap;
  std::vector<StringPiece> carray;
  size_t i = 0;
  // parse lines
  std::vector<StringPiece> lines;
  split('\n', conf, lines);
  for (auto& line : lines) {
    line = line.subpiece(0, line.find('#'));
    if (line.empty())
      continue;
    if (line.subpiece(0, 2) != "0x") {
      auto c = trimWhitespace(line);
      if (c.size() >= kMaxCategoryLength) {
        RDDLOG(ERROR) << "category[" << c << "] too long";
        return false;
      }
      if (cmap.find(c) != cmap.end()) {
        RDDLOG(ERROR) << "category[" << c << "] is already defined";
        return false;
      }
      cmap.emplace(c, CharInfo(i++));
      carray.push_back(c);
    } else {
      CharRange range = parseCharRangeFromLine(line);
      if (range.c.empty())
        continue;
      ranges.push_back(range);
    }
  }
  // build char info table
  std::vector<CharInfo> table(0xffff);
  for (auto& range : ranges) {
    CharInfo ci = encode(range.c, cmap);
    for (char16_t c = range.l; c <= range.h; ++c) {
      table[c] |= ci;
    }
  }
  if (charset == "gbk") {
    std::vector<CharInfo> gbk_table(0xffff);
    for (size_t c = 0; c < gbk_table.size(); ++c) {
      gbk_table[gUnicodeToGBKTable[c]] = table[c];
    }
    table.swap(gbk_table);
  }
  else if (charset == "ucs2") {
    // do nothing
  }
  else {
    RDDLOG(ERROR) << "illegal encoding: " << charset;
    return false;
  }
  // write char property file
  std::string out;
  uint32_t n = carray.size();
  out.append((char*)&n, sizeof(n));
  for (auto& c : carray) {
    out.append(c.str());
    out.append(kMaxCategoryLength - c.size(), '\0');
  }
  out.append((char*)&table[0], sizeof(CharInfo) * table.size());
  if (!writeFile(out, ofile.c_str(), 0644)) {
    RDDLOG(ERROR) << "write char property file " << ofile << " failed";
    return false;
  }
  return true;
}

bool PeckerCharProperty::open(const std::string& filename) {
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

} // namespace rdd
