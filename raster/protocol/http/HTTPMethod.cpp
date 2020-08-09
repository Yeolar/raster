/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/protocol/http/HTTPMethod.h"

#include <vector>

#include "accelerator/Conv.h"
#include "raster/thread/UnionBasedStatic.h"
#include "raster/protocol/http/HTTPException.h"

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

namespace raster {

HTTPMethod stringToMethod(acc::StringPiece method) {
  int index = 0;
  for (auto& cur : s_methodStrings.data) {
    //if (caseInsensitiveEqual(cur, method)) {
      return HTTPMethod(index);
    //}
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

} // namespace raster
