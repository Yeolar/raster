/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/protocol/http/HTTPMessage.h"
#include "raster/protocol/http/ParseURL.h"
#include "raster/protocol/http/Util.h"
#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "raster/util/MapUtil.h"
#include "raster/util/String.h"
#include "raster/util/ThreadUtil.h"
#include "raster/util/UnionBasedStatic.h"

namespace rdd {

namespace RFC2616 {

BodyAllowed isRequestBodyAllowed(HTTPMethod method) {
  if (method == HTTPMethod::TRACE) {
    return BodyAllowed::NOT_ALLOWED;
  }
  if (method == HTTPMethod::OPTIONS ||
      method == HTTPMethod::POST ||
      method == HTTPMethod::PUT ||
      method == HTTPMethod::CONNECT) {
    return BodyAllowed::DEFINED;
  }
  return BodyAllowed::NOT_DEFINED;
}

bool responseBodyMustBeEmpty(unsigned status) {
  return (status == 304 || status == 204 || (100 <= status && status < 200));
}

bool bodyImplied(const HTTPHeaders& headers) {
  return headers.exists(HTTP_HEADER_TRANSFER_ENCODING) ||
    headers.exists(HTTP_HEADER_CONTENT_LENGTH);
}

bool parseQvalues(StringPiece value, std::vector<TokenQPair> &output) {
  bool result = true;
  static ThreadLocal<std::vector<StringPiece>> tokens;
  tokens->clear();
  split(",", value, *tokens, true /*ignore empty*/);
  for (auto& token: *tokens) {
    auto pos = token.find(';');
    double qvalue = 1.0;
    if (pos != std::string::npos) {
      auto qpos = token.find("q=", pos);
      if (qpos != std::string::npos) {
        StringPiece qvalueStr(token.data() + qpos + 2,
                              token.size() - (qpos + 2));
        try {
          qvalue = to<double>(&qvalueStr);
        } catch (const std::range_error&) {
          // q=<some garbage>
          result = false;
        }
        // we could validate that the remainder of qvalueStr was all whitespace,
        // for now we just discard it
      } else {
        // ; but no q=
        result = false;
      }
      token.reset(token.start(), pos);
    }
    // strip leading whitespace
    while (token.size() > 0 && isspace(token[0])) {
      token.reset(token.start() + 1, token.size() - 1);
    }
    if (token.size() == 0) {
      // empty token
      result = false;
    } else {
      output.emplace_back(token, qvalue);
    }
  }
  return result && output.size() > 0;
}

} // namespace RFC2616

namespace {

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
    return HTTPMessage::createUrl(scheme, authority, path, query, fragment);
  authority = b.authority();
  if (path.subpiece(0, 1) == "/")
    return HTTPMessage::createUrl(scheme, authority, path, query, fragment);
  if (path.empty()) {
    path = b.path();
    if (query.empty())
      query = b.query();
    return HTTPMessage::createUrl(scheme, authority, path, query, fragment);
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
  return HTTPMessage::createUrl(
      scheme, authority, join('/', segments), query, fragment);
}

std::string urlConcat(const std::string& url,
                      const std::map<std::string, std::string>& params) {
  if (params.empty())
    return url;

  std::string out = url;
  char c = url.back();
  if (c != '?' && c != '&') {
    out.push_back(url.rfind('?') != std::string::npos ? '&' : '?');
  }
  for (auto it = params.begin(); it != params.end(); it++) {
    if (it != params.begin()) {
      out.push_back('&');
    }
    uriEscape(it->first, out, UriEscapeMode::QUERY);
    out.push_back('=');
    uriEscape(it->second, out, UriEscapeMode::QUERY);
  }
  out.shrink_to_fit();
  return out;
}

/*
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
    HTTPHeaders headers;
    headers.parse(part.subpiece(0, eoh));
    part.advance(eoh + 4);
    std::map<std::string, std::string> dispParams;
    auto dispHeader = headers.combine(HTTP_HEADER_CONTENT_DISPOSITION);
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
      auto ctype = headers.getSingle(HTTP_HEADER_CONTENT_TYPE,
                                     "application/unknown");
      files.emplace(name, HTTPFile{filename, value.str(), ctype});
    }
  }
}
*/

} // namespace rdd
