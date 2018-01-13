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

#include "uconv.h"

#include <cstring>

namespace rdd {
namespace uconv {

#define MAX_GBK_LENGTH      24066
#define MAX_UNICODE_LENGTH  65536
#define MAX_PSEDO_UTF8      1024
#define FIRST_CHAR_OFFSET   0x0081
#define SECOND_CHAR_OFFSET  0x0040

uint16_t gGBKToUnicodeTable[MAX_GBK_LENGTH] = {
#include "cp936map.h"
};

uint16_t gUnicodeToGBKTable[MAX_UNICODE_LENGTH] = {
#include "un2gbkmap.h"
};

uint16_t gPsedoUtf8[MAX_PSEDO_UTF8] = {
#include "psedoutf8.h"
};

#define IS_PSEDOUTF8(x) (((x) < MAX_PSEDO_UTF8) && ((x) > 0) && gPsedoUtf8[(x)])

inline uint16_t gbkCharToUnicode(char a, char b) {
  uint16_t idx = ((uint8_t)a - FIRST_CHAR_OFFSET) * 0x00bf
                + (uint8_t)b - SECOND_CHAR_OFFSET;
  if (idx < MAX_GBK_LENGTH) {
    return gGBKToUnicodeTable[idx];
  } else {
    return UCS2_NO_MAPPING;
  }
}

#define CHK_FLG                             \
  do {                                      \
    if (flags & UCONV_INVCHAR_ERROR)        \
      return -1;                            \
    else if (flags & UCONV_INVCHAR_REPLACE) \
      *dst = UCS2_NO_MAPPING;               \
  } while (0)

int gbkToUnicode(const char *src,
                 unsigned int srcLength,
                 uint16_t *dst,
                 unsigned int dstSize,
                 int flags) {
  unsigned int i = 0;
  unsigned int dl = 0;

  if (!src || !dst || !dstSize) {
    return 0;
  }
  for (i = 0; i < srcLength && dl < dstSize; ++i) {
    // 第一个字节为汉字高字节
    if (LEGAL_GBK_FIRST_BYTE(*src)) {
      // 最后的半个汉字
      if (i + 1 >= srcLength) {
        break;
      }
      // 有效的GBK
      if (LEGAL_GBK_SECOND_BYTE(src[1])) {
        *dst = gbkCharToUnicode(src[0], src[1]);
        // here *dst must be a valid unicode char
        src += 2;
        i++;
      } else {
        // TODO
        // 判断 GB18030 的情况？
        CHK_FLG;
        src++;
      }
    } else {
      if (IS_ASCII(*src)) {
        *dst = CAST_CHAR_TO_UNICHAR(*src);
      } else if (IS_GBK_EURO(*src)) {
        *dst = UCS2_EURO;
      } else {
        CHK_FLG;
      }
      src++;
    }
    dst++;
    dl++;
  }
  return dl;
}

static const char gHexArray[] = "0123456789ABCDEF";

inline uint16_t unicodeCharToGBK(uint16_t unicode,
                                 char *firstChar,
                                 char *secondChar) {
  *firstChar = gUnicodeToGBKTable[unicode] >> 8;
  *secondChar = gUnicodeToGBKTable[unicode] & 0xff;
  return gUnicodeToGBKTable[unicode];
}

int unicodeToGBK(const uint16_t *src,
                 unsigned int srcLength,
                 char *dst,
                 unsigned int dstSize,
                 int flags,
                 uint16_t replaceChar) {
  unsigned int di = 0, i = 0;
  char *dstNext = dst;

  if (!src || !dst || !dstSize) {
    return 0;
  }

  for (i = 0; i < srcLength && di < dstSize - 1; ++i, ++src) {
    // ASCII码
    if (IS_ASCII(*src)) {
      *dstNext = CAST_UNICHAR_TO_CHAR(*src);
      ++di;
      ++dstNext;
    }
    // 转换到最后一个字符，空间不够摆放一个汉字，则舍弃之
    else if ((di + 2) >= dstSize) {
      break;
    }
    // 转换成功
    else if (unicodeCharToGBK(*src, dstNext, dstNext + 1)) {
      di += 2;
      dstNext += 2;
    }
    // hack: for euro sign
    else if (IS_UCS2_EURO(*src)) {
      *dstNext = GBK_EURO;
      ++di;
      ++dstNext;
    }
    // 转换失败
    else {
      switch (flags) {
      // 使用一个特定字符替换
      case UCONV_INVCHAR_REPLACE:
        if (replaceChar < 256) {
          *dstNext = CAST_UNICHAR_TO_CHAR(replaceChar);
          ++dstNext;
          ++di;
        } else {
          *dstNext = replaceChar >> 8;
          *(dstNext + 1) = replaceChar & 0xff;
          di += 2;
          dstNext += 2;
        }
        break;
      // 返回失败
      case UCONV_INVCHAR_ERROR:
        return -1;
      // 使用实体字符代替
      case UCONV_INVCHAR_ENTITES:
        if ((di + 8) >= dstSize) {  // 实体字符 &#xXXXX; 8个字符
          break;
        }
        *dstNext++ = '&';
        *dstNext++ = '#';
        *dstNext++ = 'x';
        *dstNext++ = gHexArray[0x0f & (((*src) >> 8) >> 4)];
        *dstNext++ = gHexArray[0x0f & ((*src) >> 8)];
        *dstNext++ = gHexArray[0x0f & (((*src) & 0xff) >> 4)];
        *dstNext++ = gHexArray[0x0f & ((*src) & 0xff)];
        *dstNext++ = ';';
        di += 8;
        break;
      // 缺省方式和忽略方式不做任何事情
      case UCONV_INVCHAR_IGNORE:
      default:
        break;
      }
    }
  }
  // 强制加\0字符。因为前面有判断，这里不会造成半个汉字的情况。
  if (di >= dstSize) {
    di = dstSize - 1;
  }
  dst[di] = 0;
  return di;
}

static inline unsigned int unicodeCharToUtf8(char *dst, uint16_t src) {
  char utf8Word[5];
  int len = 0, j;
  uint16_t val = 0;

  utf8Word[4] = 0;

  if (dst == NULL || src == 0) {
    return 0;
  }

  dst[0] = 0;
  // deal with one byte ascii
  if (src < 128) {
    dst[0] = src;
    dst[1] = 0;
    return 1;
  }
  // deal with multi byte word
  else {
    for (j = 3;; j--) {
      val = src & 0x3f;
      if (src == 0)
        break;
      utf8Word[j] = val | 0x80;
      src = src >> 6;
    }

    // add utf-8 header
    len = 3 - j;
    switch (len) {
    case 2:
      utf8Word[j + 1] |= 0xc0;
      break;
    case 3:
      utf8Word[j + 1] |= 0xe0;
      break;
    default:
      return 0;
    }
    std::strcpy(dst, &utf8Word[j + 1]);
    return len;
  }
}

int gbkToUtf8(const char *src,
              unsigned int srcLength,
              char *dst,
              unsigned int dstSize,
              int flags) {
  char utf8Word[6];
  unsigned int i = 0, dl = 0, len;
  uint16_t tmp = 0;

  if (!src || !dst || !dstSize) {
    return 0;
  }

  if (dstSize < srcLength / 2 * 3 + 1) {
    return -1;
  }

  for (i = 0; i < srcLength && dl < dstSize; ++i) {
    // 第一个字节为汉字高字节
    if (LEGAL_GBK_FIRST_BYTE(*src)) {
      // 最后的半个汉字
      if (i + 1 >= srcLength) {
        break;
      }
      // 有效的GBK
      if (LEGAL_GBK_SECOND_BYTE(src[1])) {
        tmp = gbkCharToUnicode(src[0], src[1]);
        src += 2;
        i++;
      } else {
        // TODO
        // 判断 GB18030 的情况？
        if (flags & UCONV_INVCHAR_ERROR) {
          return -1;
        }
        if (flags == UCONV_INVCHAR_IGNORE) {
          src++;
          continue;
        }
      }
    } else {
      if (IS_ASCII(*src)) {
        tmp = CAST_CHAR_TO_UNICHAR(*src);
        src++;
      } else if (IS_GBK_EURO(*src)) {
        tmp = UCS2_EURO;
        src++;
      } else {
        if (flags & UCONV_INVCHAR_ERROR) {
          return -1;
        }
        if (flags == UCONV_INVCHAR_IGNORE) {
          // 注意INGORE不能用&的方式判断，因为它的值0
          src++;
          continue;
        }
      }
    }

    len = unicodeCharToUtf8(utf8Word, tmp);
    if (dl + len >= dstSize)
      break;

    std::strncpy(dst, utf8Word, len);
    dst += len;
    dl += len;
  }
  return dl;
}

int utf8ToGBK(const char *src,
              unsigned int srcLength,
              char *dst,
              unsigned int dstSize,
              int flags,
              char replaceChar) {
  static const uint8_t BytesFromHeader[256] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x00
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x10
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x20
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x30
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x40
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x50
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x60
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x70
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x80
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x90
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xA0
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xB0
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // 0xC0
      2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // 0xD0
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // 0xE0
      4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0   // 0xF0
  };

  static const uint8_t UTF8_HEADER[] = {
      0x00, /* unused */
      0x00, /* 1 byte */
      0xC0, /* 2 bytes */
      0xE0, /* 3 bytes */
      0xF0, /* 4 bytes */
      0xF8, /* 5 bytes */
      0xFC, /* 6 bytes */
  };

  unsigned int i = 0, dl = 0, len;
  uint8_t cur = 0;
  uint16_t tmp;
  char *dstOrig = dst;

  if (!src || !dst || !dstSize) {
    return 0;
  }

  if (dstSize < srcLength) {
    return -1;
  }

  for (i = 0; i < srcLength && dl < dstSize - 1;) {
    cur = src[i++];
    len = BytesFromHeader[cur];

    // skip illegal byte
    // bear in mind that UTF-8 never uses 0xFE or 0xFF
    if (len == 0)
      continue;

    // ignore the last uncomplete utf-8 word
    if (i + len - 1 > srcLength)
      break;

    // translate the next word to unicode
    tmp = cur & ~UTF8_HEADER[len];
    for (; len > 1; len--) {
      cur = src[i++];
      tmp = (tmp << 6) | (cur & 0x3F);
    }

    if (IS_ASCII(tmp)) {
      *dst++ = CAST_UNICHAR_TO_CHAR(tmp);
      dl++;
    } else if (dl + 2 >= dstSize) {
      break;
    } else if (unicodeCharToGBK(tmp, dst, dst + 1)) {
      dl += 2;
      dst += 2;
    } else if (IS_UCS2_EURO(tmp)) {
      *dst++ = GBK_EURO;
      dl++;
    } else {
      if (flags & UCONV_INVCHAR_ENTITES) {
        if ((dl + 8) >= dstSize) {  // 实体字符 &#xXXXX; 8个字符
          break;
        }
        *dst++ = '&';
        *dst++ = '#';
        *dst++ = 'x';
        *dst++ = gHexArray[0x0f & ((tmp >> 8) >> 4)];
        *dst++ = gHexArray[0x0f & (tmp >> 8)];
        *dst++ = gHexArray[0x0f & ((tmp & 0xff) >> 4)];
        *dst++ = gHexArray[0x0f & (tmp & 0xff)];
        *dst++ = ';';
        dl += 8;
      } else {
        if (flags & UCONV_INVCHAR_ERROR) {
          return -1;
        } else {
          *dst++ = replaceChar;
          dl++;
        }
      }
    }
  }
  if (dl >= dstSize) {
    dl = dstSize - 1;
  }
  dstOrig[dl] = 0;
  return dl;
}

int isUtf8(const char *src, unsigned int len, bool checkLastChar) {
  int flag = 0;
  uint16_t gbkOrUtf8 = 0, idx = 0;
  const uint8_t *str = (const uint8_t *)src;
  unsigned int asciiLen = 0;

  if (src == NULL || *src == '\0')
    return 0;

  // XXX: 对于全是英文字符的字符串，认为不是UTF-8

  if (checkLastChar) {
    while (*str) {
      if (IS_UTF8_1ST_OF_1(*str)) {
        str++;
        asciiLen++;
      } else if (IS_UTF8_1ST_OF_2(*str)) {
        str++;
        if (!IS_UTF8_2ND_THRU_6TH(*str)) {
          if (!(*str))
            flag = 2;
          break;
        }
        flag = 1;
        str++;
        /* 判断是否是貌似utf8的gbk
         * 会被判断为utf8的gbk的字符有如下特征:
         * first byte [C2, D1] ; second byte [80,bf]
         */
        if ((*(str - 2) > 0xC1) && (*(str - 2) < 0xD2) &&
            (*(str - 1) > 0x7F) && (*(str - 1) < 0xC0)) {
          /* 0xC0 - 0x80 = 64 */
          idx = (*(str - 2) - 0xC2) * 64 + (*(str - 1) - 0x80);
          if (IS_PSEDOUTF8(idx)) {
            gbkOrUtf8++;
          }
        }
      } else if (IS_UTF8_1ST_OF_3(*str)) {
        str++;
        if (!IS_UTF8_2ND_THRU_6TH(*str)) {
          if (!(*str))
            flag = 2;
          break;
        }
        if (!IS_UTF8_2ND_THRU_6TH(*(str + 1))) {
          if (!(*(str + 1))) {
            flag = 2;
          } else {
            flag = 3;
          }
          break;  // return 0;
        }
        flag = 1;
        str += 2;
      } else if (IS_UTF8_1ST_OF_4(*str)) {
        str++;
        if (!IS_UTF8_2ND_THRU_6TH(*str)) {
          if (!(*str)) {
            flag = 2;
          }
          break;  // return 0;
        }
        if (!IS_UTF8_2ND_THRU_6TH(*(str + 1))) {
          if (!(*(str + 1))) {
            flag = 2;
          }
          break;  // return 0;
        }
        if (!IS_UTF8_2ND_THRU_6TH(*(str + 2))) {
          if (!(*(str + 2)))
            flag = 2;
          break;  // return 0;
        }
        flag = 1;
        str += 3;
      } else {
        flag = 0;
        break;  // return 0;
      }
    }
  } else {
    while (*str) {
      if (IS_UTF8_1ST_OF_1(*str)) {
        str++;
        asciiLen++;
      } else if (IS_UTF8_1ST_OF_2(*str)) {
        str++;
        if (!IS_UTF8_2ND_THRU_6TH(*str))
          return 0;
        str++;
        flag = 1;
        /* 判断是否是貌似utf8的gbk
         * 会被判断为utf8的gbk的字符有如下特征:
         * first byte [C2, D1] ; second byte [80,bf]
         */
        if ((*(str - 2) > 0xC1) && (*(str - 2) < 0xD2) &&
            (*(str - 1) > 0x7F) && (*(str - 1) < 0xC0)) {
          /* 0xC0 - 0x80 = 64 */
          idx = (*(str - 2) - 0xC2) * 64 + (*(str - 1) - 0x80);
          if (IS_PSEDOUTF8(idx)) {
            gbkOrUtf8++;
          }
        }
      } else if (IS_UTF8_1ST_OF_3(*str)) {
        str++;
        if (!IS_UTF8_2ND_THRU_6TH(*str) || !IS_UTF8_2ND_THRU_6TH(*(str + 1)))
          return 0;
        str += 2;
        flag = 1;
      } else if (IS_UTF8_1ST_OF_4(*str)) {
        str++;
        if (!IS_UTF8_2ND_THRU_6TH(*str) ||
            !IS_UTF8_2ND_THRU_6TH(*(str + 1)) ||
            !IS_UTF8_2ND_THRU_6TH(*(str + 2)))
          return 0;
        str += 3;
        flag = 1;
      } else {
        return 0;
      }
    }
  }

  if (gbkOrUtf8 && ((unsigned int)gbkOrUtf8 << 1) == (len - asciiLen)) {
    return 0;
  }

  if ((flag == 2) && ((len - asciiLen) > 4) && checkLastChar) {
    if (str > (const uint8_t *)src)
      str--;
    if ((unsigned int)((str - (const uint8_t *)src) + 4) >= len) {
      flag = 1; /* 判断是utf8 */
    }
  }

  if (flag == 1)
    return str - (const uint8_t *)src; /* 返回判断到的字节数，便于做截断 */
  else
    return 0;
}

bool isGBK(const char *src) {
  if (src == NULL || *src == '\0')
    return false;

  while (*src) {
    if (LEGAL_GBK_FIRST_BYTE(*src)) {
      /* 最后的半个汉字 */
      if (*(src + 1) == '\0')
        return true;

      if (LEGAL_GBK_SECOND_BYTE(*(src + 1))) {
        src += 2;
      } else {
        return false;
      }
    } else if (IS_ASCII(*src) || IS_GBK_EURO(*src)) {
      src++;
    } else {
      return false;
    }
  }
  return true;
}

bool isGBK(const char *src, int length) {
  if (src == NULL || *src == '\0') {
    return false;
  }
  while (*src && length > 0) {
    if (LEGAL_GBK_FIRST_BYTE(*src)) {
      /* 最后的半个汉字 */
      if (*(src + 1) == '\0') {
        return true;
      }
      if (LEGAL_GBK_SECOND_BYTE(*(src + 1))) {
        src += 2;
        length -= 2;
      } else {
        return false;
      }
    } else if (IS_ASCII(*src) || IS_GBK_EURO(*src)) {
      src++;
      length--;
    } else {
      return false;
    }
  }
  return true;
}

} // namespace uconv
} // namespace rdd
