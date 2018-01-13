/*
 * Copyright 2017 Facebook, Inc.
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

/**
 *
 * Serialize and deserialize rdd::dynamic values as JSON.
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

#include "raster/util/dynamic.h"
#include "raster/util/Range.h"

namespace rdd {

//////////////////////////////////////////////////////////////////////

namespace json {

struct serialization_opts {
  explicit serialization_opts()
      : allow_non_string_keys(false),
        javascript_safe(false),
        pretty_formatting(false),
        encode_non_ascii(false),
        validate_utf8(false),
        allow_trailing_comma(false),
        sort_keys(false),
        skip_invalid_utf8(false),
        allow_nan_inf(false),
        double_mode(double_conversion::DoubleToStringConverter::SHORTEST),
        double_num_digits(0), // ignored when mode is SHORTEST
        double_fallback(false),
        parse_numbers_as_strings(false),
        recursion_limit(100) {}

  // If true, keys in an object can be non-strings.  (In strict
  // JSON, object keys must be strings.)  This is used by dynamic's
  // operator<<.
  bool allow_non_string_keys;

  /*
   * If true, refuse to serialize 64-bit numbers that cannot be
   * precisely represented by fit a double---instead, throws an
   * exception if the document contains this.
   */
  bool javascript_safe;

  // If true, the serialized json will contain space and newlines to
  // try to be minimally "pretty".
  bool pretty_formatting;

  // If true, non-ASCII utf8 characters would be encoded as \uXXXX.
  bool encode_non_ascii;

  // Check that strings are valid utf8
  bool validate_utf8;

  // Allow trailing comma in lists of values / items
  bool allow_trailing_comma;

  // Sort keys of all objects before printing out (potentially slow)
  bool sort_keys;

  // Replace invalid utf8 characters with U+FFFD and continue
  bool skip_invalid_utf8;

  // true to allow NaN or INF values
  bool allow_nan_inf;

  // Options for how to print floating point values.  See Conv.h
  // toAppend implementation for floating point for more info
  double_conversion::DoubleToStringConverter::DtoaMode double_mode;
  unsigned int double_num_digits;

  // Fallback to double when a value that looks like integer is too big to
  // fit in an int64_t. Can result in loss a of precision.
  bool double_fallback;

  // Do not parse numbers. Instead, store them as strings and leave the
  // conversion up to the user.
  bool parse_numbers_as_strings;

  // Recursion limit when parsing.
  unsigned int recursion_limit;
};

/*
 * Main JSON serialization routine taking rdd::dynamic parameters.
 * For the most common use cases there are simpler functions in the
 * main rdd namespace below.
 */
std::string serialize(dynamic const&, serialization_opts const&);

/*
 * Escape a string so that it is legal to print it in JSON text and
 * append the result to out.
 */

void escapeString(
    StringPiece input,
    std::string& out,
    const serialization_opts& opts);

/*
 * Strip all C99-like comments (i.e. // and / * ... * /)
 */
std::string stripComments(StringPiece jsonC);

/*
 * Some extension.
 */
template <class T>
inline T jto(const dynamic& j) { return j; }

template <> inline int32_t     jto(const dynamic& j) { return j.asInt(); }
template <> inline uint32_t    jto(const dynamic& j) { return j.asInt(); }
template <> inline int64_t     jto(const dynamic& j) { return j.asInt(); }
template <> inline uint64_t    jto(const dynamic& j) { return j.asInt(); }
template <> inline float       jto(const dynamic& j) { return j.asDouble(); }
template <> inline double      jto(const dynamic& j) { return j.asDouble(); }
template <> inline bool        jto(const dynamic& j) { return j.asBool(); }
template <> inline std::string jto(const dynamic& j) { return j.asString(); }

template <class T>
inline T get(const dynamic& j, const std::string& key,
             const T& dflt = T()) {
  return jto<T>(j.getDefault(key, dflt));
}

inline std::string get(const dynamic& j, const std::string& key,
                       const char* dflt = "") {
  return get<std::string>(j, key, dflt);
}

template <class T>
inline std::vector<T> getArray(const dynamic& j, const std::string& key) {
  std::vector<T> v;
  for (auto& i : j.at(key)) {
    v.push_back(jto<T>(i));
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

//////////////////////////////////////////////////////////////////////

/*
 * Parse a json blob out of a range and produce a dynamic representing
 * it.
 */
dynamic parseJson(StringPiece, json::serialization_opts const&);
dynamic parseJson(StringPiece);

/*
 * Serialize a dynamic into a json string.
 */
std::string toJson(dynamic const&);

/*
 * Same as the above, except format the json with some minimal
 * indentation.
 */
std::string toPrettyJson(dynamic const&);

/*
 * Printer for GTest.
 * Uppercase name to fill GTest's API, which calls this method through ADL.
 */
void PrintTo(const dynamic&, std::ostream*);
//////////////////////////////////////////////////////////////////////

} // namespace rdd
