/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <exception>
#include <stdarg.h>
#include <string>
#include <vector>

#include "raster/util/Conv.h"
#include "raster/util/Demangle.h"
#include "raster/util/Range.h"

#define RDD_PRINTF_FORMAT_ATTR(format_param, dots_param) \
  __attribute__((__format__(__printf__, format_param, dots_param)))

namespace rdd {

/**
 * stringPrintf is much like printf but deposits its result into a
 * string. Two signatures are supported: the first simply returns the
 * resulting string, and the second appends the produced characters to
 * the specified string and returns a reference to it.
 */
std::string stringPrintf(const char* format, ...) RDD_PRINTF_FORMAT_ATTR(1, 2);

void stringPrintf(std::string* out, const char* fmt, ...)
  RDD_PRINTF_FORMAT_ATTR(2, 3);

std::string& stringAppendf(std::string* output, const char* format, ...)
  RDD_PRINTF_FORMAT_ATTR(2, 3);

/**
 * Similar to stringPrintf, but accepts a va_list argument.
 *
 * As with vsnprintf() itself, the value of ap is undefined after the call.
 * These functions do not call va_end() on ap.
 */
std::string stringVPrintf(const char* format, va_list ap);
void stringVPrintf(std::string* out, const char* format, va_list ap);
std::string& stringVAppendf(std::string* out, const char* format, va_list ap);

/**
 * Same functionality as Python's binascii.hexlify.
 * Returns true on successful conversion.
 *
 * If append_output is true, append data to the output rather than
 * replace it.
 */
template <class InputString>
bool hexlify(const InputString& input, std::string& output,
             bool append = false);

/**
 * Same functionality as Python's binascii.unhexlify.
 * Returns true on successful conversion.
 */
template <class InputString>
bool unhexlify(const InputString& input, std::string& output);

/**
 * Return a fbstring containing the description of the given errno value.
 * Takes care not to overwrite the actual system errno, so calling
 * errnoStr(errno) is valid.
 */
std::string errnoStr(int err);

/**
 * Debug string for an exception: include type and what(), if
 * defined.
 */
inline std::string exceptionStr(const std::exception& e) {
  return to<std::string>(demangle(typeid(e)), ": ", e.what());
}

template<typename E>
auto exceptionStr(const E& e)
  -> typename std::enable_if<!std::is_base_of<std::exception, E>::value,
                             std::string>::type
{
  return to<std::string>(demangle(typeid(e)));
}

/**
 * Split a string into a list of tokens by delimiter.
 *
 * The split interface here supports different output types, selected
 * at compile time: StringPiece or std::string.  If you are
 * using a vector to hold the output, it detects the type based on
 * what your vector contains.  If the output vector is not empty, split
 * will append to the end of the vector.
 *
 * You can also use splitTo() to write the output to an arbitrary
 * OutputIterator (e.g. std::inserter() on a std::set<>), in which
 * case you have to tell the function the type.  (Rationale:
 * OutputIterators don't have a value_type, so we can't detect the
 * type in splitTo without being told.)
 *
 * Examples:
 *
 *   std::vector<rdd::StringPiece> v;
 *   rdd::split(":", "asd:bsd", v);
 *
 *   std::set<StringPiece> s;
 *   rdd::splitTo<StringPiece>(":", "asd:bsd:asd:csd",
 *     std::inserter(s, s.begin()));
 *
 * Split also takes a flag (ignoreEmpty) that indicates whether adjacent
 * delimiters should be treated as one single separator (ignoring empty tokens)
 * or not (generating empty tokens).
 */

template <class Delim, class String, class OutputType>
void split(const Delim& delimiter, const String& input,
           std::vector<OutputType>& out,
           bool ignoreEmpty = false);

template <class OutputValueType, class Delim, class String,
          class OutputIterator>
void splitTo(const Delim& delimiter, const String& input,
             OutputIterator out,
             bool ignoreEmpty = false);

/*
 * Split a string into a fixed number of string pieces and/or numeric types
 * by delimiter. Any numeric type that rdd::to<> can convert to from a
 * string piece is supported as a target. Returns 'true' if the fields were
 * all successfully populated.  Returns 'false' if there were too few fields
 * in the input, or too many fields if exact=true.  Casting exceptions will
 * not be caught.
 *
 * Examples:
 *
 *  rdd::StringPiece name, key, value;
 *  if (rdd::split('\t', line, name, key, value))
 *  ...
 *
 *  rdd::StringPiece name;
 *  double value;
 *  int id;
 *  if (rdd::split('\t', line, name, value, id))
 *  ...
 *
 * The 'exact' template parameter specifies how the function behaves when too
 * many fields are present in the input string. When 'exact' is set to its
 * default value of 'true', a call to split will fail if the number of fields in
 * the input string does not exactly match the number of output parameters
 * passed. If 'exact' is overridden to 'false', all remaining fields will be
 * stored, unsplit, in the last field, as shown below:
 *
 *  rdd::StringPiece x, y.
 *  if (rdd::split<false>(':', "a:b:c", x, y))
 *  assert(x == "a" && y == "b:c");
 *
 * Note that this will likely not work if the last field's target is of numeric
 * type, in which case rdd::to<> will throw an exception.
 */
template <class T>
struct IsSplitTargetType {
  enum {
    value = std::is_arithmetic<T>::value ||
        std::is_same<T, StringPiece>::value ||
        std::is_same<T, std::string>::value
  };
};

template <bool exact = true, class Delim, class OutputType,
          class... OutputTypes>
typename std::enable_if<IsSplitTargetType<OutputType>::value, bool>::type
split(const Delim& delimiter, StringPiece input,
      OutputType& outHead, OutputTypes&... outTail);

/*
 * Similar to split(), but split for any char matching of delimiters.
 */
template <class Delim, class String, class OutputType>
void splitAny(const Delim& delimiters, const String& input,
              std::vector<OutputType>& out,
              bool ignoreEmpty = false);

template <class OutputValueType, class Delim, class String,
          class OutputIterator>
void splitAnyTo(const Delim& delimiters, const String& input,
                OutputIterator out,
                bool ignoreEmpty = false);

/**
 * Join list of tokens.
 *
 * Stores a string representation of tokens in the same order with
 * deliminer between each element.
 */

template <class Delim, class Iterator>
void join(const Delim& delimiter, Iterator begin, Iterator end,
          std::string& output);

template <class Delim, class Container>
void join(const Delim& delimiter, const Container& container,
          std::string& output) {
  join(delimiter, container.begin(), container.end(), output);
}

template <class Delim, class Value>
void join(const Delim& delimiter, const std::initializer_list<Value>& values,
          std::string& output) {
  join(delimiter, values.begin(), values.end(), output);
}

template <class Delim, class Container>
std::string join(const Delim& delimiter, const Container& container) {
  std::string output;
  join(delimiter, container.begin(), container.end(), output);
  return output;
}

template <class Delim, class Value>
std::string join(const Delim& delimiter,
                 const std::initializer_list<Value>& values) {
  std::string output;
  join(delimiter, values.begin(), values.end(), output);
  return output;
}

template <class Delim, class Iterator>
std::string join(const Delim& delimiter, Iterator begin, Iterator end) {
  std::string output;
  join(delimiter, begin, end, output);
  return output;
}

template <class Delim, class Iterator>
void joinMap(const Delim& delimiter, const Delim& pairDelimiter,
             Iterator begin, Iterator end,
             std::string& output);

template <class Delim, class Container>
std::string joinMap(const Delim& delimiter, const Delim& pairDelimiter,
                    const Container& container) {
  std::string output;
  joinMap(delimiter, pairDelimiter, container.begin(), container.end(), output);
  return output;
}

template <class Delim, class Iterator>
std::string joinMap(const Delim& delimiter, const Delim& pairDelimiter,
                    Iterator begin, Iterator end) {
  std::string output;
  joinMap(delimiter, pairDelimiter, begin, end, output);
  return output;
}

/**
 * Returns a subpiece with all whitespace removed from the front.
 * Whitespace means any of [' ', '\n', '\r', '\t'].
 */
StringPiece ltrimWhitespace(StringPiece sp);

/**
 * Returns a subpiece with all whitespace removed from the back.
 * Whitespace means any of [' ', '\n', '\r', '\t'].
 */
StringPiece rtrimWhitespace(StringPiece sp);

/**
 * Returns a subpiece with all whitespace removed from the back and front.
 * Whitespace means any of [' ', '\n', '\r', '\t'].
 */
inline StringPiece trimWhitespace(StringPiece sp) {
  return ltrimWhitespace(rtrimWhitespace(sp));
}

/**
 * Fast, in-place lowercasing of ASCII alphabetic characters in strings.
 * Leaves all other characters unchanged, including those with the 0x80
 * bit set.
 * @param str String to convert
 * @param len Length of str, in bytes
 */
void toLowerAscii(char* str, size_t length);

template <class String>
void toLowerAscii(String& s) {
  for (auto& c : s) {
    c = tolower(c);
  }
}

} // namespace rdd

#include "String-inl.h"
