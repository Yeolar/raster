/*
 * Copyright (C) 2017, Yeolar
 */

#include <boost/regex.hpp>

#include "raster/protocol/http/Cookie.h"
#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "raster/util/String.h"
#include "raster/util/UnionBasedStatic.h"

namespace rdd {

namespace {

// These quoting routines conform to the RFC2109 specification, which in
// turn references the character definitions from RFC2068.  They provide
// a two-way quoting algorithm.  Any non-text character is translated
// into a 4 character sequence: a forward-slash followed by the
// three-digit octal equivalent of the character.  Any '\' or '"' is
// quoted with a preceding '\' slash.
//
// Because of the way browsers really handle cookies (as opposed
// to what the RFC says) we also encode , and ;
//
static const int kLegalChars[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0,   // !#$%&'*+-.
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,   // 0-9
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   // A-O
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,   // P-Z^_
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,   // `a-o
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0,   // p-z|~
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const int kTranslator[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,   // ,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,   // ;
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static int isLegalChar(int c) {
  return c >= 0 ? kLegalChars[c] : 0;
}

typedef std::vector<std::string> StringVector;
DEFINE_UNION_STATIC_CONST_NO_INIT(StringVector, Vector, sReserved);

__attribute__((__constructor__))
void initReserved() {
  new (const_cast<StringVector*>(&sReserved.data)) StringVector {
    "Comment",
    "Domain",
    "expires",
    "httponly",
    "Max-Age",
    "Path",
    "secure",
    "Version",
  };
}

static std::string formatDate(int future = 0) {
  time_t t = time(nullptr) + future;
  char buff[1024];
  tm timeTupple;
  gmtime_r(&t, &timeTupple);
  strftime(buff, 1024, "%a, %d %b %Y %H:%M:%S %Z", &timeTupple);
  return std::string(buff);
}

}

bool CookieMorsel::isReserved(StringPiece k) {
  for (auto& s : sReserved.data) {
    if (caseInsensitiveEqual(s, k))
      return true;
  }
  return false;
}

void CookieMorsel::set(
    const std::string& k, const std::string& v, const std::string& cv) {
  if (isReserved(k))
    throw std::invalid_argument("Attempt to set a reserved key: " + k);
  if (!std::all_of(k.begin(), k.end(), isLegalChar))
    throw std::invalid_argument("Illegal key: " + k);
  key = k;
  value = v;
  codedValue = cv;
}

void CookieMorsel::setAttr(const std::string& k, const std::string& v) {
  for (auto& s : sReserved.data) {
    if (caseInsensitiveEqual(s, k)) {
      attributes[s] = v;
      return;
    }
  }
  throw std::invalid_argument("Invalid attribute: " + k);
}

std::string CookieMorsel::str() const {
  std::string out;
  toAppend("Set-Cookie: ", key, '=', codedValue, &out);

  for (auto& kv : attributes) {
    if (kv.second.empty())
      continue;
    if (kv.first == "expires" &&
        std::all_of(kv.second.begin(), kv.second.end(), isdigit)) {
      toAppend("; expires=", formatDate(to<int>(kv.second)), &out);
    } else if (kv.first == "secure" || kv.first == "httponly") {
      toAppend("; ", kv.first, &out);
    } else {
      toAppend("; ", kv.first, '=', kv.second, &out);
    }
  }
  return out;
}

std::string Cookie::quote(StringPiece sp) {
  // If the string does not need to be double-quoted,
  // then just return the string.  Otherwise, surround
  // the string in doublequotes and precede quote (with a \)
  // special characters.
  if (std::all_of(sp.begin(), sp.end(), isLegalChar))
    return sp.str();

  std::string output;
  output.push_back('"');

  for (const uint8_t& c : sp) {
    if (c == '"' || c == '\\') {
      output.push_back('\\');
      output.push_back(c);
    } else if (kTranslator[c]) {
      output.push_back('\\');
      output.push_back('0' + ((c >> 6) & 07));
      output.push_back('0' + ((c >> 3) & 07));
      output.push_back('0' + ( c       & 07));
    } else {
      output.push_back(c);
    }
  }
  output.push_back('"');

  return output;
}

std::string Cookie::unquote(StringPiece sp) {
  // If there aren't any doublequotes,
  // then there can't be any special characters.  See RFC 2109.
  if (sp.size() < 2)
    return sp.str();
  if (sp.front() != '"' || sp.back() != '"')
    return sp.str();

  sp.pop_front();
  sp.pop_back();

  auto p = sp.begin();
  auto e = sp.end();
  std::string output;

  while (p < e) {
    auto s = p;
    while (p != e && *p != '\\') p++;
    if (p != s) {
      output.append(s, p - s);
    }
    if (p != e) p++;  // jump '\\'
    if (p == e) break;
    if (p + 2 < e) {
      uint8_t c1 = *p;
      uint8_t c2 = *(p + 1);
      uint8_t c3 = *(p + 2);
      if ((c1 >= '0' && c1 <= '3') &&
          (c2 >= '0' && c2 <= '7') &&
          (c3 >= '0' && c3 <= '7')) {
        output.push_back(((c1 - '0') << 6) + ((c2 - '0') << 3) + (c3 - '0'));
        p += 3;
      }
    }
  }

  return output;
}

void Cookie::set(
    const std::string& k, const std::string& v, const std::string& cv) {
  auto p = data.emplace(k, std::make_shared<CookieMorsel>());
  p.first->second->set(k, v, cv);
}

void Cookie::set(const std::string& key, const std::string& value) {
  set(key, value, quote(value));
}

#define LEGAL_CHARS R"([\w\d!#%&'~_`><@,:/\$\*\+\-\.\^\|\)\(\?\}\{\=])"

void Cookie::load(StringPiece sp) {
  static const boost::regex cookieRegex(
    "("                         // key:
      LEGAL_CHARS "+?"          // Any word of at least one letter, nongreedy
    ")"
      R"(\s*=\s*)"              // Equal Sign
    "("                         // val:
      R"(\"(?:[^\\\"]|\\.)*\")" // Any doublequoted string
      "|"                       // or
      R"(\w{3},\s[\s\w\d-]{9,11}\s[\d:]{8}\sGMT)" // Special case for "expires"
      "|"                       // or
      LEGAL_CHARS "*"           // Any word or empty string
    ")"
      R"(\s*;?)");              // Probably ending in a semi-colon

  std::shared_ptr<CookieMorsel> cm;

  while (!sp.empty()) {
    boost::cmatch match;
    if (!boost::regex_search(sp.begin(), sp.end(), match, cookieRegex)) {
      break;
    }
    StringPiece key(match[1].first, match[1].second);
    StringPiece val(match[2].first, match[2].second);

    sp.advance(match.length(0));

    // Parse the key and value in case it's metainfo
    if (key.front() == '$') {
      // Ignore attributes which pertain to the cookie mechanism
      // as a whole. See RFC 2109.
      key.pop_front();
      if (cm) cm->setAttr(key.str(), val.str());
    } else if (CookieMorsel::isReserved(key)) {
      if (cm) cm->setAttr(key.str(), unquote(val));
    } else {
      auto k = key.str();
      set(k, unquote(val), val.str());
      cm = data[k];
    }
  }
}

std::string Cookie::str() const {
  std::vector<std::string> v;
  for (auto& kv : data) {
    v.emplace_back(std::move(kv.second->str()));
  }
  return join("\r\n", v);
}

}  // namespace rdd
