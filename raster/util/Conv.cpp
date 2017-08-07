/*
 * Copyright (C) 2017, Yeolar
 */

#define RDD_CONV_INTERNAL
#include "raster/util/Conv.h"

namespace rdd {
namespace detail {

extern const char digit1[101] =
  "00000000001111111111222222222233333333334444444444"
  "55555555556666666666777777777788888888889999999999";
extern const char digit2[101] =
  "01234567890123456789012345678901234567890123456789"
  "01234567890123456789012345678901234567890123456789";

template <> const char *const MaxString<bool>::value = "true";
template <> const char *const MaxString<uint8_t>::value = "255";
template <> const char *const MaxString<uint16_t>::value = "65535";
template <> const char *const MaxString<uint32_t>::value = "4294967295";
template <> const char *const MaxString<unsigned long>::value = "18446744073709551615";  // sizeof(unsigned long) == 8
static_assert(sizeof(unsigned long) >= 4,
              "Wrong value for MaxString<unsigned long>::value,"
              " please update.");
template <> const char *const MaxString<unsigned long long>::value =
  "18446744073709551615";
static_assert(sizeof(unsigned long long) >= 8,
              "Wrong value for MaxString<unsigned long long>::value"
              ", please update.");

inline bool bool_str_cmp(const char** b, size_t len, const char* value) {
  // Can't use strncasecmp, since we want to ensure that the full value matches
  const char* p = *b;
  const char* e = *b + len;
  const char* v = value;
  while (*v != '\0') {
    if (p == e || tolower(*p) != *v) { // value is already lowercase
      return false;
    }
    ++p;
    ++v;
  }

  *b = p;
  return true;
}

bool str_to_bool(StringPiece* src) {
  auto b = src->begin(), e = src->end();
  for (;; ++b) {
    RDD_RANGE_CHECK_STRINGPIECE(
      b < e, "No non-whitespace characters found in input string", *src);
    if (!isspace(*b)) break;
  }

  bool result;
  size_t len = e - b;
  switch (*b) {
    case '0':
    case '1': {
      result = false;
      for (; b < e && isdigit(*b); ++b) {
        RDD_RANGE_CHECK_STRINGPIECE(
          !result && (*b == '0' || *b == '1'),
          "Integer overflow when parsing bool: must be 0 or 1", *src);
        result = (*b == '1');
      }
      break;
    }
    case 'y':
    case 'Y':
      result = true;
      if (!bool_str_cmp(&b, len, "yes")) {
        ++b;  // accept the single 'y' character
      }
      break;
    case 'n':
    case 'N':
      result = false;
      if (!bool_str_cmp(&b, len, "no")) {
        ++b;
      }
      break;
    case 't':
    case 'T':
      result = true;
      if (!bool_str_cmp(&b, len, "true")) {
        ++b;
      }
      break;
    case 'f':
    case 'F':
      result = false;
      if (!bool_str_cmp(&b, len, "false")) {
        ++b;
      }
      break;
    case 'o':
    case 'O':
      if (bool_str_cmp(&b, len, "on")) {
        result = true;
      } else if (bool_str_cmp(&b, len, "off")) {
        result = false;
      } else {
        RDD_RANGE_CHECK_STRINGPIECE(false, "Invalid value for bool", *src);
      }
      break;
    default:
      RDD_RANGE_CHECK_STRINGPIECE(false, "Invalid value for bool", *src);
  }

  src->assign(b, e);
  return result;
}

} // namespace detail
} // namespace rdd

