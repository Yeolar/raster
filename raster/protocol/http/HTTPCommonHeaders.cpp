/* C++ code produced by gperf version 3.0.4 */
/* Command-line: gperf -m5 --output-file=HTTPCommonHeaders.cpp  */
/* Computed positions: -k'1,8,22,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif


/*
 * Copyright (c) 2015, Facebook, Inc.
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

#include <cstring>

#include "raster/protocol/http/HTTPCommonHeaders.h"
#include "accelerator/Logging.h"

namespace raster {

struct HTTPCommonHeaderName { const char* name; uint8_t code; };


/* the following is a placeholder for the build script to generate a list
 * of enum values from the list in HTTPCommonHeaders.txt
 */;
enum
  {
    TOTAL_KEYWORDS = 83,
    MIN_WORD_LENGTH = 2,
    MAX_WORD_LENGTH = 37,
    MIN_HASH_VALUE = 4,
    MAX_HASH_VALUE = 113
  };

/* maximum key range = 110, duplicates = 0 */

#ifndef GPERF_DOWNCASE
#define GPERF_DOWNCASE 1
static unsigned char gperf_downcase[256] =
  {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
     30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
     45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
     60,  61,  62,  63,  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255
  };
#endif

#ifndef GPERF_CASE_MEMCMP
#define GPERF_CASE_MEMCMP 1
static int
gperf_case_memcmp (register const char *s1, register const char *s2, register unsigned int n)
{
  for (; n > 0;)
    {
      unsigned char c1 = gperf_downcase[(unsigned char)*s1++];
      unsigned char c2 = gperf_downcase[(unsigned char)*s2++];
      if (c1 == c2)
        {
          n--;
          continue;
        }
      return (int)c1 - (int)c2;
    }
  return 0;
}
#endif

class HTTPCommonHeadersInternal
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct HTTPCommonHeaderName *in_word_set (const char *str, unsigned int len);
};

inline unsigned int
HTTPCommonHeadersInternal::hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114,   0, 114, 114, 114, 114,
      114, 114, 114,   0, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114,   3, 114,   4,  34,   0,
       46,  46,  19,  24, 114,  60,  17,  32,   8,  20,
       24, 114,   0,   6,   2,  62,  52,  36,   6,  22,
       12, 114, 114, 114, 114, 114, 114,   3, 114,   4,
       34,   0,  46,  46,  19,  24, 114,  60,  17,  32,
        8,  20,  24, 114,   0,   6,   2,  62,  52,  36,
        6,  22,  12, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114, 114, 114, 114, 114,
      114, 114, 114, 114, 114, 114
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[21]];
      /*FALLTHROUGH*/
      case 21:
      case 20:
      case 19:
      case 18:
      case 17:
      case 16:
      case 15:
      case 14:
      case 13:
      case 12:
      case 11:
      case 10:
      case 9:
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      /*FALLTHROUGH*/
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

static const unsigned char lengthtable[] =
  {
     0,  0,  0,  0,  2,  5,  3,  7,  6,  7,  6,  6,  6,  7,
     8, 11, 12, 13,  0, 13, 16, 15, 13, 14, 16,  4,  7, 15,
    16, 22, 16, 19,  8,  6,  6, 15, 13, 14,  4,  3, 12,  8,
    29, 17,  5, 30, 10, 16, 22, 32,  4,  3, 15, 15, 13, 25,
    13, 11,  3,  0, 28,  0, 27,  9, 15, 17, 16,  9, 16,  7,
     8, 16, 29, 28, 10, 15, 10, 19,  4,  3,  0,  4,  4,  0,
     0,  0, 12, 17,  0,  7, 37,  0,  0,  0, 10,  0,  0,  0,
    13,  0,  0,  0,  0,  0, 18,  0,  0,  0,  0,  0,  0,  0,
     0, 19
  };

static const struct HTTPCommonHeaderName wordlist[] =
  {
    {""}, {""}, {""}, {""},
    {"TE", HTTP_HEADER_TE},
    {"Range", HTTP_HEADER_RANGE},
    {"Age", HTTP_HEADER_AGE},
    {"Referer", HTTP_HEADER_REFERER},
    {"Expect", HTTP_HEADER_EXPECT},
    {"Trailer", HTTP_HEADER_TRAILER},
    {"Cookie", HTTP_HEADER_COOKIE},
    {"Accept", HTTP_HEADER_ACCEPT},
    {"Server", HTTP_HEADER_SERVER},
    {"Expires", HTTP_HEADER_EXPIRES},
    {"X-Scheme", HTTP_HEADER_X_SCHEME},
    {"Content-MD5", HTTP_HEADER_CONTENT_MD5},
    {"Content-Type", HTTP_HEADER_CONTENT_TYPE},
    {"Content-Range", HTTP_HEADER_CONTENT_RANGE},
    {""},
    {"X-Wap-Profile", HTTP_HEADER_X_WAP_PROFILE},
    {"Content-Language", HTTP_HEADER_CONTENT_LANGUAGE},
    {"X-Forwarded-For", HTTP_HEADER_X_FORWARDED_FOR},
    {"Accept-Ranges", HTTP_HEADER_ACCEPT_RANGES},
    {"Accept-Charset", HTTP_HEADER_ACCEPT_CHARSET},
    {"X-Accel-Redirect", HTTP_HEADER_X_ACCEL_REDIRECT},
    {"Host", HTTP_HEADER_HOST},
    {"Refresh", HTTP_HEADER_REFRESH},
    {"X-Frame-Options", HTTP_HEADER_X_FRAME_OPTIONS},
    {"Content-Location", HTTP_HEADER_CONTENT_LOCATION},
    {"Access-Control-Max-Age", HTTP_HEADER_ACCESS_CONTROL_MAX_AGE},
    {"X-XSS-Protection", HTTP_HEADER_X_XSS_PROTECTION},
    {"Content-Disposition", HTTP_HEADER_CONTENT_DISPOSITION},
    {"If-Range", HTTP_HEADER_IF_RANGE},
    {"Pragma", HTTP_HEADER_PRAGMA},
    {"Origin", HTTP_HEADER_ORIGIN},
    {"Accept-Language", HTTP_HEADER_ACCEPT_LANGUAGE},
    {"Authorization", HTTP_HEADER_AUTHORIZATION},
    {"Content-Length", HTTP_HEADER_CONTENT_LENGTH},
    {"Date", HTTP_HEADER_DATE},
    {"DNT", HTTP_HEADER_DNT},
    {"X-Powered-By", HTTP_HEADER_X_POWERED_BY},
    {"Location", HTTP_HEADER_LOCATION},
    {"Access-Control-Expose-Headers", HTTP_HEADER_ACCESS_CONTROL_EXPOSE_HEADERS},
    {"X-Forwarded-Proto", HTTP_HEADER_X_FORWARDED_PROTO},
    {"Allow", HTTP_HEADER_ALLOW},
    {"Access-Control-Request-Headers", HTTP_HEADER_ACCESS_CONTROL_REQUEST_HEADERS},
    {"Connection", HTTP_HEADER_CONNECTION},
    {"X-Requested-With", HTTP_HEADER_X_REQUESTED_WITH},
    {"X-Content-Type-Options", HTTP_HEADER_X_CONTENT_TYPE_OPTIONS},
    {"Access-Control-Allow-Credentials", HTTP_HEADER_ACCESS_CONTROL_ALLOW_CREDENTIALS},
    {"ETag", HTTP_HEADER_ETAG},
    {"P3P", HTTP_HEADER_P3P},
    {"Accept-Datetime", HTTP_HEADER_ACCEPT_DATETIME},
    {"X-UA-Compatible", HTTP_HEADER_X_UA_COMPATIBLE},
    {"Cache-Control", HTTP_HEADER_CACHE_CONTROL},
    {"Strict-Transport-Security", HTTP_HEADER_STRICT_TRANSPORT_SECURITY},
    {"If-None-Match", HTTP_HEADER_IF_NONE_MATCH},
    {"Retry-After", HTTP_HEADER_RETRY_AFTER},
    {"Via", HTTP_HEADER_VIA},
    {""},
    {"Access-Control-Allow-Headers", HTTP_HEADER_ACCESS_CONTROL_ALLOW_HEADERS},
    {""},
    {"Access-Control-Allow-Origin", HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN},
    {"X-Real-IP", HTTP_HEADER_X_REAL_IP},
    {"Accept-Encoding", HTTP_HEADER_ACCEPT_ENCODING},
    {"Transfer-Encoding", HTTP_HEADER_TRANSFER_ENCODING},
    {"Content-Encoding", HTTP_HEADER_CONTENT_ENCODING},
    {"Timestamp", HTTP_HEADER_TIMESTAMP},
    {"Proxy-Connection", HTTP_HEADER_PROXY_CONNECTION},
    {"Upgrade", HTTP_HEADER_UPGRADE},
    {"If-Match", HTTP_HEADER_IF_MATCH},
    {"WWW-Authenticate", HTTP_HEADER_WWW_AUTHENTICATE},
    {"Access-Control-Request-Method", HTTP_HEADER_ACCESS_CONTROL_REQUEST_METHOD},
    {"Access-Control-Allow-Methods", HTTP_HEADER_ACCESS_CONTROL_ALLOW_METHODS},
    {"User-Agent", HTTP_HEADER_USER_AGENT},
    {"Front-End-Https", HTTP_HEADER_FRONT_END_HTTPS},
    {"Set-Cookie", HTTP_HEADER_SET_COOKIE},
    {"If-Unmodified-Since", HTTP_HEADER_IF_UNMODIFIED_SINCE},
    {"Vary", HTTP_HEADER_VARY},
    {"VIP", HTTP_HEADER_VIP},
    {""},
    {"Link", HTTP_HEADER_LINK},
    {"From", HTTP_HEADER_FROM},
    {""}, {""}, {""},
    {"Max-Forwards", HTTP_HEADER_MAX_FORWARDS},
    {"If-Modified-Since", HTTP_HEADER_IF_MODIFIED_SINCE},
    {""},
    {"Warning", HTTP_HEADER_WARNING},
    {"X-Content-Security-Policy-Report-Only", HTTP_HEADER_X_CONTENT_SECURITY_POLICY_REPORT_ONLY},
    {""}, {""}, {""},
    {"Keep-Alive", HTTP_HEADER_KEEP_ALIVE},
    {""}, {""}, {""},
    {"Last-Modified", HTTP_HEADER_LAST_MODIFIED},
    {""}, {""}, {""}, {""}, {""},
    {"Proxy-Authenticate", HTTP_HEADER_PROXY_AUTHENTICATE},
    {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
    {"Proxy-Authorization", HTTP_HEADER_PROXY_AUTHORIZATION}
  };

const struct HTTPCommonHeaderName *
HTTPCommonHeadersInternal::in_word_set (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key].name;

            if ((((unsigned char)*str ^ (unsigned char)*s) & ~32) == 0 && !gperf_case_memcmp (str, s, len))
              return &wordlist[key];
          }
    }
  return 0;
}


HTTPHeaderCode HTTPCommonHeaders::hash(const char* name, size_t len) {
  const HTTPCommonHeaderName* match =
    HTTPCommonHeadersInternal::in_word_set(name, len);
  return (match == nullptr) ? HTTP_HEADER_OTHER : HTTPHeaderCode(match->code);
}

std::string* HTTPCommonHeaders::initHeaderNames() {
  DCHECK_LE(MAX_HASH_VALUE, 255);
  auto headerNames = new std::string[256];
  for (int j = MIN_HASH_VALUE; j <= MAX_HASH_VALUE; ++j) {
    uint8_t code = wordlist[j].code;
    const uint8_t OFFSET = 2; // first 2 values are reserved for special cases
    if (code >= OFFSET && code < TOTAL_KEYWORDS + OFFSET
                       && wordlist[j].name[0] != '\0') {
      DCHECK_EQ(headerNames[code], std::string());
      // this would mean a duplicate header code in the .gperf file

      headerNames[code] = wordlist[j].name;
    }
  }
  return headerNames;
}

} // namespace raster
