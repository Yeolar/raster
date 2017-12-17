/*
 * Copyright (C) 2017, Yeolar
 */

#include <vector>

#include "raster/protocol/http/HTTPException.h"
#include "raster/protocol/http/HTTPMethod.h"
#include "raster/util/Conv.h"
#include "raster/util/UnionBasedStatic.h"

#define HTTP_METHOD_STR(method) #method

namespace {

// Method strings. This is a union-based static because this structure is
// accessed from multiple threads and still needs to be accessible after exit()
// is called to avoid crashing.
typedef std::vector<std::string> StringVector;
DEFINE_UNION_STATIC_CONST_NO_INIT(StringVector, Vector, s_methodStrings);

__attribute__((__constructor__))
void initMethodStrings() {
  new (const_cast<StringVector*>(&s_methodStrings.data)) StringVector {
    HTTP_METHOD_GEN(HTTP_METHOD_STR)
  };
}

}

namespace rdd {

HTTPMethod stringToMethod(StringPiece method) {
  int index = 0;
  for (auto& cur : s_methodStrings.data) {
    if (caseInsensitiveEqual(cur, method)) {
      return HTTPMethod(index);
    }
    index++;
  }
  throw HTTPException(405, method);
}

const std::string& methodToString(HTTPMethod method) {
  return s_methodStrings.data[static_cast<unsigned>(method)];
}

std::ostream& operator <<(std::ostream& out, HTTPMethod method) {
  out << methodToString(method);
  return out;
}

} // namespace rdd
