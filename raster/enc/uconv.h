/*
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

/*
 * Ripped from mozilla intl/uconv.
 */

#pragma once

#include <cstdint>

#define UCS2_NO_MAPPING         ((uint16_t)0xfffd)
#define IS_ASCII(a)             (0 == (0xff80 & (a)))
/* XXX: 0xa2e3 is euro sign in gbk standard but 0x80 in microsoft's cp936map */
#define GBK_EURO                ((char)0x80)
#define IS_GBK_EURO(c)          ((char)0x80 == (c))  // EURO SIGN
#define UCS2_EURO               ((uint16_t)0x20ac)
#define IS_UCS2_EURO(c)         ((uint16_t)0x20ac == (c))

#define CAST_CHAR_TO_UNICHAR(a) ((uint16_t)((uint8_t)(a)))
#define CAST_UNICHAR_TO_CHAR(a) ((char)a)

#define UINT8_IN_RANGE(min, c, max) \
  (((uint8_t)(c) >= (uint8_t)(min)) && ((uint8_t)(c) <= (uint8_t)(max)))

#define LEGAL_GBK_MULTIBYTE_FIRST_BYTE(c) (UINT8_IN_RANGE(0x81, (c), 0xFE))
#define FIRST_BYTE_IS_SURROGATE(c)        (UINT8_IN_RANGE(0x90, (c), 0xFE))
#define LEGAL_GBK_2BYTE_SECOND_BYTE(c)    (UINT8_IN_RANGE(0x40, (c), 0x7E) || \
                                           UINT8_IN_RANGE(0x80, (c), 0xFE))
#define LEGAL_GBK_4BYTE_SECOND_BYTE(c)    (UINT8_IN_RANGE(0x30, (c), 0x39))
#define LEGAL_GBK_4BYTE_THIRD_BYTE(c)     (UINT8_IN_RANGE(0x81, (c), 0xFE))
#define LEGAL_GBK_4BYTE_FORTH_BYTE(c)     (UINT8_IN_RANGE(0x30, (c), 0x39))

#define LEGAL_GBK_FIRST_BYTE(c)           (UINT8_IN_RANGE(0x81, (c), 0xFE))
#define LEGAL_GBK_SECOND_BYTE(c)          (UINT8_IN_RANGE(0x40, (c), 0x7E) || \
                                           UINT8_IN_RANGE(0x80, (c), 0xFE))

/*
 * UTF8 defines and macros
 * The following is ripped from libi18n/unicvt.c and include files
 */
#define _1_OCTET_BASE   0x00  /* 0xxxxxxx */
#define _1_OCTET_MASK   0x7F  /* x1111111 */
#define _C_OCTET_BASE   0x80  /* 10xxxxxx continuing */
#define _C_OCTET_MASK   0x3F  /* 00111111 continuing */
#define _2_OCTET_BASE   0xC0  /* 110xxxxx */
#define _2_OCTET_MASK   0x1F  /* 00011111 */
#define _3_OCTET_BASE   0xE0  /* 1110xxxx */
#define _3_OCTET_MASK   0x0F  /* 00001111 */
#define _4_OCTET_BASE   0xF0  /* 11110xxx */
#define _4_OCTET_MASK   0x07  /* 00000111 */
#define _5_OCTET_BASE   0xF8  /* 111110xx */
#define _5_OCTET_MASK   0x03  /* 00000011 */
#define _6_OCTET_BASE   0xFC  /* 1111110x */
#define _6_OCTET_MASK   0x01  /* 00000001 */

#define IS_UTF8_1ST_OF_1(x)     (((x) & ~_1_OCTET_MASK) == _1_OCTET_BASE)
#define IS_UTF8_1ST_OF_2(x)    ((((x) & ~_2_OCTET_MASK) == _2_OCTET_BASE) && \
                                 ((x) != 0xC0) && \
                                 ((x) != 0xC1))
#define IS_UTF8_1ST_OF_3(x)     (((x) & ~_3_OCTET_MASK) == _3_OCTET_BASE)
#define IS_UTF8_1ST_OF_4(x)     (((x) & ~_4_OCTET_MASK) == _4_OCTET_BASE)
#define IS_UTF8_1ST_OF_5(x)     (((x) & ~_5_OCTET_MASK) == _5_OCTET_BASE)
#define IS_UTF8_1ST_OF_6(x)     (((x) & ~_6_OCTET_MASK) == _6_OCTET_BASE)
#define IS_UTF8_2ND_THRU_6TH(x) (((x) & ~_C_OCTET_MASK) == _C_OCTET_BASE)
#define IS_UTF8_1ST_OF_UCS2(x)  IS_UTF8_1ST_OF_1(x) || \
                                IS_UTF8_1ST_OF_2(x) || \
                                IS_UTF8_1ST_OF_3(x)

namespace rdd {
namespace uconv {

enum {
  UCONV_INVCHAR_IGNORE  = 0x0,  // 忽略无效的字符
  UCONV_INVCHAR_REPLACE = 0x1,  // 使用特殊字符代替无效的字符
  UCONV_INVCHAR_ERROR   = 0x2,  // 遇到无效字符时返回失败
  UCONV_INVCHAR_ENTITES = 0x4,  // 把无效的字符转换为html实体字符
};

/**
 * Convert GBK encoding string to unicode string.
 * Return converted unicode chars count, -1 on error.
 *
 * flags define behavior when meet invalid chars:
 *  - UCONV_INVCHAR_REPLACE, replace with UCS2_NO_MAPPING
 *  - UCONV_INVCHAR_ERROR, return -1
 */
int gbkToUnicode(const char *src,
                 unsigned int srcLength,
                 uint16_t *dst,
                 unsigned int dstSize,
                 int flags);

/**
 * Convert unicode string to GBK encoding string,
 * append \0 at end and delete half-Chinese character at last if any.
 * Return converted GBK chars byte length.
 *
 * flags define behavior when meet invalid chars:
 *  - UCONV_INVCHAR_IGNORE, ignore
 *  - UCONV_INVCHAR_REPLACE, replace with replaceChar
 *  - UCONV_INVCHAR_ERROR, return -1
 *  - UCONV_INVCHAR_ENTITES, replace with HTML entities
 */
int unicodeToGBK(const uint16_t *src,
                 unsigned int srcLength,
                 char *dst,
                 unsigned int dstSize,
                 int flags,
                 unsigned short replaceChar);

/**
 * Convert GBK encoding string to UTF-8 encoding string.
 * Return converted UTF-8 chars byte length, -1 on error.
 *
 * flags define behavior when meet invalid chars:
 *  - UCONV_INVCHAR_IGNORE, ignore
 *  - UCONV_INVCHAR_ERROR, return -1
 */
int gbkToUtf8(const char *src,
              unsigned int srcLength,
              char *dst,
              unsigned int dstSize,
              int flags);

/**
 * Convert UTF-8 encoding string to GBK encoding string.
 * Return converted GBK encoding byte length, -1 on error.
 *
 * flags define behavior when meet invalid chars:
 *  - UCONV_INVCHAR_REPLACE, replace with replaceChar
 *  - UCONV_INVCHAR_ERROR, return -1
 *  - UCONV_INVCHAR_ENTITES, replace with HTML entities
 */
int utf8ToGBK(const char *src,
              unsigned int srcLength,
              char *dst,
              unsigned int dstSize,
              int flags,
              char replaceChar);

/**
 * If string is UTF-8 encoding, return valid UTF-8 chars byte length
 * checked (maybe less than len), else return 0.
 *
 * checkLastChar = true allow the last char not match UTF8,
 * used for cutted string such as too long URL (cut by browser),
 * suggest false for other situations.
 *
 * NOTE: full alpha-char string is not considered as UTF-8 encoding.
 */
int isUtf8(const char *str, unsigned int len, bool checkLastChar);

/**
 * If string is GBK encoding.
 */
bool isGBK(const char *src);
bool isGBK(const char *src, int length);

} // namespace uconv
} // namespace rdd
