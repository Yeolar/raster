/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/ParseURL.h"
#include "raster/protocol/http/Util.h"
#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "raster/util/MapUtil.h"
#include "raster/util/String.h"
#include "raster/util/UnionBasedStatic.h"

namespace rdd {

namespace {

typedef std::map<int, std::string> CodeMap;
DEFINE_UNION_STATIC_CONST_NO_INIT(CodeMap, Map, sW3CCodeMap);

__attribute__((__constructor__))
void initW3CCodeMap() {
  new (const_cast<CodeMap*>(&sW3CCodeMap.data)) CodeMap {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing (WEBDAV)"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi Status (WEBDAV)"},
    {226, "Im Used (Delta encoding)"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Requested Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {422, "Unprocessable Entity (WEBDAV)"},
    {423, "Locked (WEBDAV)"},
    {424, "Failed Dependency (WEBDAV)"},
    {426, "Upgrade Required (Upgrade to TLS)"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {507, "Insufficient Storage (WEBDAV)"},
    {510, "Not Extended (An Extension Framework)"},
  };
}

std::string unquote(StringPiece sp) {
  if (sp.size() < 2)
    return sp.str();
  if (sp.front() != '"' || sp.back() != '"')
    return sp.str();

  std::string out;
  sp.pop_front();
  sp.pop_back();
  auto i = sp.find('\\');

  while (i < sp.size() - 1) {
    toAppend(sp.subpiece(0, i), &out);
    char c = sp[i + 1];
    if (c != '\\' && c != '"') {
      toAppend('\\', &out);
    }
    toAppend(c, &out);
    sp.advance(i + 2);
    i = sp.find('\\');
  }

  toAppend(sp, &out);
  return out;
}

}

bool isValidResponseCode(int code) {
  return contain(sW3CCodeMap.data, code);
}

std::string getResponseW3CName(int code) {
  return get_default(sW3CCodeMap.data, code, "Unknown");
}

std::string urlJoin(const std::string& base, const std::string& url) {
  if (base.empty()) return url;
  if (url.empty()) return base;

  ParseURL b(base);
  ParseURL u(url);
  StringPiece scheme = u.scheme();
  StringPiece authority = u.authority();
  StringPiece path = u.path();
  StringPiece query = u.query();
  StringPiece fragment = u.fragment();

  if (scheme != b.scheme())
    return url;
  if (!authority.empty())
    return urlUnparse(scheme, authority, path, query, fragment);
  authority = b.authority();
  if (path.subpiece(0, 1) == "/")
    return urlUnparse(scheme, authority, path, query, fragment);
  if (path.empty()) {
    path = b.path();
    if (query.empty())
      query = b.query();
    return urlUnparse(scheme, authority, path, query, fragment);
  }

  std::vector<StringPiece> segments;
  split('/', b.path(), segments);
  segments.pop_back();
  split('/', path, segments);

  if (segments.back() == ".") {
    segments.back() = "";
  }
  remove(segments, ".");
  while (true) {
    int i = 1;
    int n = segments.size() - 1;
    while (i < n) {
      if (segments[i] == ".." &&
          segments[i-1] != "" &&
          segments[i-1] != "..") {
        segments.erase(segments.begin() + i-1, segments.begin() + i+1);
        break;
      }
      i++;
    }
    if (i >= n)
      break;
  }
  if (segments.size() == 2 &&
      segments.front() == "" &&
      segments.back() == "..") {
    segments.back() = "";
  } else if (segments.size() >= 2 && segments.back() == "..") {
    segments.pop_back();
    segments.back() = "";
  }
  return urlUnparse(scheme, authority, join('/', segments), query, fragment);
}

std::string urlConcat(const std::string& url, const URLQuery& query) {
  if (query.empty()) return url;

  std::string out(url);
  char c = url.back();
  if (c != '?' && c != '&') {
    out.push_back(url.find('?') != std::string::npos ? '&' : '?');
  }
  return out + encodeQuery(query);
}

static StringPiece splitParam(StringPiece& sp) {
  auto count = [](StringPiece s, StringPiece p) -> int {
    int n = 0;
    size_t r = s.find(p);
    while (r != StringPiece::npos) {
      n++;
      s.advance(r + p.size());
      r = s.find(p);
    }
    return n;
  };

  size_t i = sp.find(';');
  while (i != StringPiece::npos) {
    auto sub = sp.subpiece(0, i);
    auto k = count(sub, "\"") - count(sub, "\\\"");
    if (k % 2 == 0) break;
    i = sp.find(';', i + 1);
  }

  StringPiece result = sp.subpiece(0, i);
  sp.advance(i != StringPiece::npos ? i + 1 : sp.size());

  return result;
}

static std::string parseHeader(
    StringPiece line, std::map<std::string, std::string>& dict) {
  std::string key = splitParam(line).str();

  while (!line.empty()) {
    auto p = splitParam(line);
    StringPiece name, value;
    if (split('=', p, name, value)) {
      dict.emplace(toLowerAscii(trimWhitespace(name)),
                   unquote(trimWhitespace(value)));
    }
  }
  return key;
}

void parseBodyArguments(
    StringPiece contentType,
    StringPiece body,
    URLQuery& arguments,
    std::multimap<std::string, HTTPFile>& files) {
  if (contentType.startsWith("application/x-www-form-urlencoded")) {
    parseQuery(arguments, body);
  } else if (contentType.startsWith("multipart/form-data")) {
    bool found = false;
    while (!contentType.empty()) {
      auto field = contentType.split_step(';');
      StringPiece k, v;
      if (split('=', trimWhitespace(field), k, v)) {
        if (k == "boundary" && !v.empty()) {
          parseMultipartFormData(v, body, arguments, files);
          found = true;
          break;
        }
      }
    }
    if (!found) {
      RDDLOG(WARN) << "Invalid multipart/form-data";
    }
  }
}

void parseMultipartFormData(
    StringPiece boundary,
    StringPiece data,
    URLQuery& arguments,
    std::multimap<std::string, HTTPFile>& files) {
  // The standard allows for the boundary to be quoted in the header,
  // although it's rare (it happens at least for google app engine xmpp).
  if (boundary.front() == '"' && boundary.back() == '"') {
    boundary.pop_front();
    boundary.pop_back();
  }

  auto last = rfind(data, StringPiece(to<std::string>("--", boundary, "--")));
  if (last == StringPiece::npos) {
    RDDLOG(WARN) << "Invalid multipart/form-data: no final boundary";
    return;
  }

  std::vector<StringPiece> parts;
  split(to<std::string>("--", boundary, "\r\n"), data.subpiece(0, last), parts);

  for (auto part : parts) {
    if (part.empty())
      continue;
    auto eoh = part.find("\r\n\r\n");
    if (eoh == StringPiece::npos) {
      RDDLOG(WARN) << "multipart/form-data missing headers";
      continue;
    }
    HTTPHeaders headers(part.subpiece(0, eoh));
    part.advance(eoh + 4);
    std::map<std::string, std::string> dispParams;
    auto dispHeader = headers.get("Content-Disposition", "");
    auto disposition = parseHeader(dispHeader, dispParams);
    if (disposition != "form-data" || !part.endsWith("\r\n")) {
      RDDLOG(WARN) << "Invalid multipart/form-data";
      continue;
    }
    auto it = dispParams.find("name");
    if (it == dispParams.end()) {
      RDDLOG(WARN) << "multipart/form-data value missing name";
      continue;
    }
    auto name = it->second;
    auto value = part.subpiece(0, part.size() - 2); // exclude \r\n
    it = dispParams.find("filename");
    if (it == dispParams.end()) {
      arguments.emplace(name, value.str());
    } else {
      auto filename = it->second;
      auto ctype = headers.get("Content-Type", "application/unknown");
      files.emplace(name, HTTPFile{filename, value.str(), ctype});
    }
  }
}

} // namespace rdd
