/*
 * Copyright 2017 Facebook, Inc.
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

/**
 *
 * Serialize and deserialize acc::dynamic values as JSON.
 *
 * Before you use this you should probably understand the basic
 * concepts in the JSON type system:
 *
 *    Value  : String | Bool | Null | Object | Array | Number
 *    String : UTF-8 sequence
 *    Object : (String, Value) pairs, with unique String keys
 *    Array  : ordered list of Values
 *    Null   : null
 *    Bool   : true | false
 *    Number : (representation unspecified)
 *
 * ... That's about it.  For more information see http://json.org or
 * look up RFC 4627.
 *
 * If your dynamic has anything illegal with regard to this type
 * system, the serializer will throw.
 *
 * @author Jordan DeLong <delong.j@fb.com>
 */

#pragma once

#include <iosfwd>
#include <string>

#include <accelerator/dynamic.h>
#include <accelerator/Range.h>

namespace acc {
namespace json {

/*
 * Some extension.
 */
template <class T>
inline T get(const dynamic& j,
             const std::string& key,
             const T& dflt = T()) {
  return T();//as<T>(j.getDefault(key, dflt));
}

inline std::string get(const dynamic& j,
                       const std::string& key,
                       const char* dflt = "") {
  return get<std::string>(j, key, dflt);
}

template <class T>
inline std::vector<T> getArray(const dynamic& j, const std::string& key) {
  std::vector<T> v;
  for (auto& i : j.at(key)) {
    v.push_back(T());//as<T>(i));
  }
  return v;
}

inline dynamic resolve(const dynamic& j, const std::string& path) {
  std::vector<StringPiece> keys;
  split('.', path, keys);
  dynamic o = j;
  for (auto& k : keys) {
    o = dynamic(o.at(k));
  }
  return o;
}

} // namespace json

} // namespace acc
