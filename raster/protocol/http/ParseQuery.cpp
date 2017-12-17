/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/ParseQuery.h"
#include "raster/util/String.h"

namespace rdd {

/**
 * quote('abc def') -> 'abc%20def'.
 *
 * Each part of a URL, e.g. the path info, the query, etc., has a
 * different set of reserved characters that must be quoted.
 *
 * RFC 2396 Uniform Resource Identifiers (URI): Generic Syntax lists
 * the following reserved characters.
 *
 * reserved = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" | "$" | ","
 *
 * Each of these characters is reserved in some component of a URL,
 * but not necessarily in all of them.
 *
 * By default, the quote function is intended for quoting the path
 * section of a URL.  Thus, it will not encode '/'.  This character
 * is reserved, but in typical usage the quote function is being
 * called on a path where the existing slash characters are used as
 * reserved characters.
 */
static std::string quote(StringPiece s, const std::string& safe = "/") {
  static char hexValues[] = "0123456789ABCDEF";
  static std::string alwaysSafe("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789_.-");

  auto issafe = [&](char c) -> bool {
    return alwaysSafe.find(c) != std::string::npos ||
           safe.find(c) != std::string::npos;
  };

  if (s.empty())
    return s.str();
  if (std::all_of(s.begin(), s.end(), issafe))
    return s.str();

  std::string out;
  out.reserve(s.size());

  for (uint8_t c : s) {
    if (issafe(c)) {
      out.push_back(c);
    } else {
      out.push_back('%');
      out.push_back(hexValues[(c >> 4) & 0xf]);
      out.push_back(hexValues[ c       & 0xf]);
    }
  }
  return out;
}

/**
 * Quote the query fragment of a URL; replacing ' ' with '+'
 */
static std::string quotep(StringPiece s, const std::string& safe = "") {
  if (s.find(' ') != StringPiece::npos) {
    auto q = quote(s, safe + ' ');
    for (auto& c : q) {
      if (c == ' ') c = '+';
    }
    return q;
  }
  return quote(s, safe);
}

std::string encodeQuery(const URLQuery& query) {
  std::string out;
  auto it = query.begin();
  if (it != query.end()) {
    toAppend(quotep(it->first), '=', quotep(it->second), &out);
  }
  while (++it != query.end()) {
    toAppend('&', quotep(it->first), '=', quotep(it->second), &out);
  }
  return out;
}

/**
 * unquote('abc%20def') -> 'abc def'
 */
static std::string unquote(StringPiece s) {
  // TODO: handle unicode
  auto unhex = [](char c) -> int {
    return c >= '0' && c <= '9' ? c - '0' :
           c >= 'A' && c <= 'F' ? c - 'A' + 10 :
           c >= 'a' && c <= 'f' ? c - 'a' + 10 :
           -1;
  };

  std::string out;
  out.reserve(s.size());

  for (size_t i = 0; i < s.size(); ) {
    if (s[i] == '%' && i + 2 < s.size()) {
      int hbits = unhex(s[i + 1]);
      int lbits = unhex(s[i + 2]);
      if (hbits >= 0 && lbits >= 0) {
        out.push_back((hbits << 4) + lbits);
        i += 3;
        continue;
      }
    }
    out.push_back(s[i++]);
  }
  return out;
}

void parseQuery(URLQuery& query, const std::string& str) {
  std::vector<StringPiece> v;
  splitAny("&;", str, v);

  for (auto& sp : v) {
    if (sp.empty())
      continue;
    StringPiece name, value;
    if (split('=', sp, name, value)) {
      if (!value.empty()) {
        query.emplace(unquote(name), unquote(value));
      }
    }
  }
}

} // namespace rdd
