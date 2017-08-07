/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/enc/CharProperty.h"
#include <map>
#include "raster/io/FileUtil.h"
#include "raster/util/Logging.h"
#include "raster/util/String.h"

namespace rdd {

#define MAX_UNICODE_LENGTH  65536
#define MAX_CATEGORY_STRLEN 32

uint16_t gUnicodeToGBKTable[MAX_UNICODE_LENGTH] = {
#include "un2gbkmap.h"
};

bool CharProperty::open(const std::string& filename) {
  if (!readFile(filename.c_str(), data_)) {
    RDDLOG(ERROR) << "read char property file " << filename << " failed";
    return false;
  }
  uint32_t n = *((uint32_t*)data_.data());
  uint32_t offset = sizeof(uint32_t);
  if (data_.length() != (offset + MAX_CATEGORY_STRLEN * n +
                         sizeof(uint32_t) * 0xffff)) {
    RDDLOG(ERROR) << "illegal size of char property file " << filename;
    return false;
  }
  categories_.clear();
  for (uint32_t i = 0; i < n; ++i) {
    categories_.push_back(&data_[offset]);
    offset += MAX_CATEGORY_STRLEN;
  }
  map_ = (const CharInfo*) &data_[offset];
  return true;
}

char16_t readHex(StringPiece s) {
  auto unhex = [](char c) -> int {
    return c >= '0' && c <= '9' ? c - '0' :
           c >= 'a' && c <= 'f' ? c - 'a' + 10 :
           c >= 'A' && c <= 'F' ? c - 'A' + 10 :
           -1;
  };
  // 0x0000 - 0xffff
  char16_t r = unhex(s[2]) * 4096
             + unhex(s[3]) * 256
             + unhex(s[4]) * 16
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
    base.type += it->second.type;
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
      if (c.size() >= MAX_CATEGORY_STRLEN) {
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
      table[c] = table[c] | ci;
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
    out.append(MAX_CATEGORY_STRLEN - c.size(), '\0');
  }
  out.append((char*)&table[0], sizeof(CharInfo) * table.size());
  if (!writeFile(out, ofile.c_str(), 0644)) {
    RDDLOG(ERROR) << "write char property file " << ofile << " failed";
    return false;
  }
  return true;
}

} // namespace rdd
