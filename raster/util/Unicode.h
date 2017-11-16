/*
 * Copyright 2017 Facebook, Inc.
 * Copyright ICU
 * Copyright (C) 2017, Yeolar
 */

// Some utility routines relating to unicode.

#pragma once

#include <algorithm>
#include <string>
#include <boost/operators.hpp>

#include "raster/util/Logging.h"
#include "raster/util/Range.h"

/**
 * Counts the trail bytes for a UTF-8 lead byte.
 * Returns 0 for 0..0xbf as well as for 0xfe and 0xff.
 *
 * This is internal since it is not meant to be called directly by external
 * clients; however it is called by public macros in this file and thus must
 * remain stable.
 *
 * Note: Beginning with ICU 50, the implementation uses a multi-condition
 * expression which was shown in 2012 (on x86-64) to compile to fast,
 * branch-free code.  leadByte is evaluated multiple times.
 *
 * The pre-ICU 50 implementation used the exported array utf8_countTrailBytes:
 * #define U8_COUNT_TRAIL_BYTES(leadByte) (utf8_countTrailBytes[leadByte])
 * leadByte was evaluated exactly once.
 *
 * @param leadByte The first byte of a UTF-8 sequence. Must be 0..0xff.
 * @internal
 */
#define U8_COUNT_TRAIL_BYTES(leadByte) \
  ((uint8_t)(leadByte)<0xf0 ? \
    ((uint8_t)(leadByte)>=0xc0)+((uint8_t)(leadByte)>=0xe0) : \
     (uint8_t)(leadByte)<0xfe ? \
      3+((uint8_t)(leadByte)>=0xf8)+((uint8_t)(leadByte)>=0xfc) : 0)

/**
 * Counts the trail bytes for a UTF-8 lead byte of a valid UTF-8 sequence.
 * The maximum supported lead byte is 0xf4 corresponding to U+10FFFF.
 * leadByte might be evaluated multiple times.
 *
 * This is internal since it is not meant to be called directly by external
 * clients; however it is called by public macros in this file and thus must
 * remain stable.
 *
 * @param leadByte The first byte of a UTF-8 sequence. Must be 0..0xff.
 * @internal
 */
#define U8_COUNT_TRAIL_BYTES_UNSAFE(leadByte) \
  (((leadByte)>=0xc0)+((leadByte)>=0xe0)+((leadByte)>=0xf0))

/**
 * Does this code unit (byte) encode a code point by itself (US-ASCII 0..0x7f)?
 * @param c 8-bit code unit (byte)
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U8_IS_SINGLE(c) (((c)&0x80)==0)

/**
 * Is this code unit (byte) a UTF-8 lead byte?
 * @param c 8-bit code unit (byte)
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U8_IS_LEAD(c) ((uint8_t)((c)-0xc0)<0x3e)

#define U8_IS_SINGLE_OR_LEAD(c) (U8_IS_SINGLE(c) || U8_IS_LEAD(c))

/**
 * Is this code unit (byte) a UTF-8 trail byte?
 * @param c 8-bit code unit (byte)
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U8_IS_TRAIL(c) (((c)&0xc0)==0x80)

/**
 * How many code units (bytes) are used for the UTF-8 encoding
 * of this Unicode code point?
 * @param c 32-bit code point
 * @return 1..4, or 0 if c is a surrogate or not a Unicode code point
 * @stable ICU 2.4
 */
#define U8_LENGTH(c) \
  ((uint32_t)(c)<=0x7f ? 1 : \
    ((uint32_t)(c)<=0x7ff ? 2 : \
      ((uint32_t)(c)<=0xd7ff ? 3 : \
        ((uint32_t)(c)<=0xdfff || (uint32_t)(c)>0x10ffff ? 0 : \
          ((uint32_t)(c)<=0xffff ? 3 : 4)\
        ) \
      ) \
    ) \
  )

/**
 * The maximum number of UTF-8 code units (bytes) per Unicode code point
 * (U+0000..U+10ffff).
 * @return 4
 * @stable ICU 2.4
 */
#define U8_MAX_LENGTH 4

namespace rdd {

///////////////////////////////////////////////////////////////////////////

template <class Iter>
size_t utf8CodePointLength(Iter p) {
  return 1 + U8_COUNT_TRAIL_BYTES_UNSAFE((uint8_t)*p);
}

/**
 * Advance the string offset from one code point boundary to the next.
 * (Post-incrementing iteration.)
 * "Unsafe", assumes well-formed UTF-8.
 *
 * @see U8_FWD_1_UNSAFE
 */
template <class Iter>
Iter utf8ForwardCodePoint(Iter p) {
  return p + 1 + U8_COUNT_TRAIL_BYTES_UNSAFE((uint8_t)*p);
}

/**
 * Advance the string offset from one code point boundary to the next.
 * (Post-incrementing iteration.)
 * "Safe", checks for illegal sequences and for string boundaries.
 *
 * @see U8_FWD_1
 */
template <class Iter>
Iter utf8ForwardCodePointSafe(Iter p, Iter end) {
  uint8_t c = (uint8_t)*p++;

  if (U8_IS_LEAD(c)) {
    ssize_t count = U8_COUNT_TRAIL_BYTES(c);
    if (p + count - end > 0) {
      return nullptr;
    }
    while (count > 0 && U8_IS_TRAIL(*p)) {
      ++p;
      --count;
    }
    if (count > 0) {
      return nullptr;
    }
  }
  return p;
}

/**
 * Move the string offset from one code point boundary to the previous one.
 * (Pre-decrementing backward iteration.)
 * The input offset may be the same as the string length.
 * "Unsafe", assumes well-formed UTF-8.
 *
 * @see U8_BACK_1_UNSAFE
 */
template <class Iter>
Iter utf8BackCodePoint(Iter p) {
  while (U8_IS_TRAIL(*--p)) {}
  return p;
}

/**
 * Move the string offset from one code point boundary to the previous one.
 * (Pre-decrementing backward iteration.)
 * The input offset may be the same as the string length.
 * "Safe", checks for string boundaries.
 */
template <class Iter>
Iter utf8BackCodePointSafe(Iter p, Iter begin) {
  while (p != begin && U8_IS_TRAIL(*--p)) {}
  if (U8_IS_TRAIL(*p)) {
    return nullptr;
  }
  return p;
}

/**
 * Iterator end of UTF-8 string.
 */
template <class Iter>
Iter utf8StringEnd(Iter begin, Iter end) {
  Iter b = begin;
  Iter e = end;
  if (b == e) {
    return e;
  }
  Iter p = utf8BackCodePointSafe(e, b);
  if (!p || e - p == utf8CodePointLength(p)) {
    return e;
  }
  return p;
}

template <class Iter, class String>
Iter utf8StringEnd(const String& str) {
  StringPiece sp(str);
  return utf8StringEnd(sp.begin(), sp.end());
}

/**
 * Check if valid of UTF-8 string.
 */
template <class Iter>
bool utf8StringIsValid(Iter begin, Iter end) {
  Iter p = begin;
  Iter e = end;
  while(p != e) {
    p = utf8ForwardCodePointSafe(p, e);
    if (!p) {
      return false;
    }
  }
  return true;
}

template <class String>
bool utf8StringIsValid(const String& str) {
  StringPiece sp(str);
  return utf8StringIsValid(sp.begin(), sp.end());
}

/**
 * Move malformed char of UTF-8 string to the end.
 * Return the begin of malformed piece at the end of string.
 */
template <class Iter>
Iter utf8StringRemoveMalformed(Iter begin, Iter end) {
  Iter to = begin;
  Iter lead = begin;
  Iter e = end;

  while (true) {
    while (lead != e) {
      uint8_t c = (uint8_t)*lead;
      if (U8_IS_SINGLE(c)) {
        to = std::copy(lead, lead + 1, to);
      } else if (U8_IS_LEAD(c)) {
        break;
      }
      ++lead;
    }
    if (lead == e)
      break;

    uint8_t count = U8_COUNT_TRAIL_BYTES(*lead);
    Iter p = lead + 1;

    while (count > 0 && p != e && U8_IS_TRAIL(*p)) {
      ++p;
      --count;
    }
    if (count == 0) {
      to = std::copy(lead, p, to);
    }
    lead = p;
  }

  return to;
}

template <class String>
std::string utf8StringRemoveMalformed(const String& str) {
  StringPiece sp(str);
  std::string out(sp.begin(), sp.end());
  auto it = utf8StringRemoveMalformed(out.begin(), out.end());
  out.erase(it, out.end());
  return out;
}

///////////////////////////////////////////////////////////////////////////

/*
 * Encode a single unicode code point into a UTF-8 byte sequence.
 *
 * Return value is undefined if `cp' is an invalid code point.
 */
std::string codePointToUtf8(char32_t cp);

/**
 * Decode a UTF-8 byte sequence into a single unicode code point.
 */
char32_t utf8ToCodePoint(const unsigned char*& p,
                         const unsigned char* const e,
                         bool skipOnError = true);

char32_t utf8ToCodePointUnsafe(const unsigned char* p);

inline char32_t utf8ToCodePoint(const StringPiece& sp) {
  DCHECK(!sp.empty());
  const unsigned char* b = (const unsigned char*)sp.begin();
  const unsigned char* e = (const unsigned char*)sp.end();
  return utf8ToCodePoint(b, e);
}

namespace unicode {

///////////////////////////////////////////////////////////////////////////
// C/POSIX.

bool isAlpha(char32_t cp);
bool isLower(char32_t cp);
bool isUpper(char32_t cp);
bool isTitle(char32_t cp);
bool isPunct(char32_t cp);
bool isDigit(char32_t cp);
bool isXDigit(char32_t cp);
bool isAlnum(char32_t cp);
bool isSpace(char32_t cp);
bool isJavaSpace(char32_t cp);
bool isWhitespace(char32_t cp);
bool isBlank(char32_t cp);
bool isCntrl(char32_t cp);
bool isISOControl(char32_t cp);
bool isGraph(char32_t cp);
bool isPrint(char32_t cp);
bool isDefined(char32_t cp);
bool isBase(char32_t cp);

int8_t getUCharType(char32_t cp);
uint32_t getUCharCategoryMask(char32_t cp);
bool isMemberOfUCharCategory(char32_t cp, uint32_t mask);

bool isNonBase(char32_t cp);

bool isWide(char32_t cp);

template <class String>
bool isWordChar(const String& str) {
  char32_t cp = utf8ToCodePoint(str);
  return cp == '_' || isAlnum(cp);
}

template <class String>
size_t blankLength(const String& str, size_t tabSize) {
  StringPiece sp(str);
  size_t n = 0;
  for (auto it = sp.begin(); it != sp.end() && isblank(*it); ++it) {
    n += *it == '\t' ? tabSize - (n % tabSize) : 1;
  }
  return n;
}

template <class Iter>
bool isBlankString(Iter first, Iter last) {
  Iter it = std::find_if_not(first, last, isblank);
  return it == last || (it + 1 == last && *it == '\n');
}

template <class String>
bool isBlankString(const String& str) {
  StringPiece sp(str);
  return isBlankString(sp.begin(), sp.end());
}

char32_t toLower(char32_t cp); // upper and title -> lower
char32_t toUpper(char32_t cp); // lower -> upper

/**
 * Convert to lowercase string.
 * Can handle non single-code point case.
 */
std::string toLower(const StringPiece& sp);

template <class String>
std::string toLower(const String& str) {
  return toLower(StringPiece(str));
}

/**
 * Convert to upper string.
 * Can handle non single-code point case.
 */
std::string toUpper(const StringPiece& sp);

template <class String>
std::string toUpper(const String& str) {
  return toUpper(StringPiece(str));
}

/**
 * Convert to opposite-case string.
 * Can handle non single-code point case.
 */
std::string toOppositeCase(const StringPiece& sp);

template <class String>
std::string toOppositeCase(const String& str) {
  return toOppositeCase(StringPiece(str));
}

/**
 * Constants for normalization modes.
 */
typedef enum {
  /** No decomposition/composition. */
  NORM_NONE = 1,
  /** Canonical decomposition. */
  NORM_NFD = 2,
  /** Compatibility decomposition. */
  NORM_NFKD = 3,
  /** Canonical decomposition followed by canonical composition. */
  NORM_NFC = 4,
  /** Compatibility decomposition followed by canonical composition. */
  NORM_NFKC =5,
  /** "Fast C or D" form. */
  NORM_FCD = 6,
  /** One more than the highest normalization mode constant. */
  NORM_MODE_COUNT
} NormalizationMode;

/**
 * Normalize a string.
 * The string will be normalized according the specified normalization mode.
 */
std::string normalize(const StringPiece& sp, NormalizationMode mode);

template <class String>
std::string normalize(const String& str, NormalizationMode mode) {
  return normalize(StringPiece(str), mode);
}

/**
 * Asciify a string.
 * Based on transliterations: any-Latin; Latin-ASCII
 */
std::string asciify(const StringPiece& sp);

template <class String>
std::string asciify(const String& str) {
  return asciify(StringPiece(str));
}

} // namespace unicode

///////////////////////////////////////////////////////////////////////////

/**
 * A CodePoint iterator over UTF-8 string.
 *
 * Assumes well-formed UTF-8.
 */
template <class Iter>
class Utf8Iterator
: public std::iterator<std::bidirectional_iterator_tag, char32_t>
, private boost::totally_ordered<Utf8Iterator<Iter>> {
public:
  Utf8Iterator() : p_() { }
  Utf8Iterator(const Iter& p) : p_(p) { }
  Utf8Iterator(const Utf8Iterator& iter) : p_(iter.p_) { }

  Utf8Iterator& operator=(const Utf8Iterator&) = default;
  Utf8Iterator& operator=(Utf8Iterator&&) = default;

  Utf8Iterator& operator++() {
    p_ = utf8ForwardCodePoint(p_);
    return *this;
  }
  Utf8Iterator operator++(int) {
    Utf8Iterator it(*this);
    operator++();
    return it;
  }

  Utf8Iterator& operator--() {
    p_ = utf8BackCodePoint(p_);
    return *this;
  }
  Utf8Iterator operator--(int) {
    Utf8Iterator it(*this);
    operator--();
    return it;
  }

  Utf8Iterator operator+(ssize_t n) const {
    Utf8Iterator it(*this);
    for (; n > 0; --n)
      ++it;
    for (; n < 0; ++n)
      --it;
    return it;
  }
  Utf8Iterator operator-(ssize_t n) const {
    return *this + -n;
  }

  ssize_t operator-(const Utf8Iterator& rhs) const {
    ssize_t n = 0;
    Utf8Iterator it(rhs);
    for (; it != *this; ++it)
      ++n;
    return n;
  }

  char32_t& operator*() {
    cp_ = utf8ToCodePointUnsafe((const unsigned char*)p_);
    return cp_;
  }
  const char32_t& operator*() const {
    cp_ = utf8ToCodePointUnsafe((const unsigned char*)p_);
    return cp_;
  }

  Iter operator&() const { return p_; }

  size_t length() const { return utf8CodePointLength(p_); }

  StringPiece piece() const {
    return StringPiece(p_, utf8CodePointLength(p_));
  }

private:
  Iter p_;
  mutable char32_t cp_;
};

template <class Iter>
inline bool operator==(const Utf8Iterator<Iter>& lhs,
                       const Utf8Iterator<Iter>& rhs) {
  return &lhs == &rhs;
}

template <class Iter>
inline bool operator<(const Utf8Iterator<Iter>& lhs,
                      const Utf8Iterator<Iter>& rhs) {
  return &lhs < &rhs;
}

template <class Iter>
Utf8Iterator<Iter> makeUtf8Iterator(const Iter& p) {
  return Utf8Iterator<Iter>(p);
}

typedef Utf8Iterator<const char*> Utf8CharIterator;

///////////////////////////////////////////////////////////////////////////

namespace u16 {

template <class Iter>
Iter advance(const Iter& first, size_t n) {
  Utf8Iterator<Iter> it(first);

  for (; n; ++it)
    n -= (*it > 0xFFFF) ? 2 : 1;

  return &it;
}

template <class Iter>
Iter advance(const Iter& first, size_t n, const Iter& last) {
  Utf8Iterator<Iter> it(first);
  Utf8Iterator<Iter> end(utf8StringEnd(first, last));
  DCHECK(&end == last);

  for (; n && it != end; ++it)
    n -= (*it > 0xFFFF) ? 2 : 1;

  return &it;
}

template <class Iter>
size_t distance(const Iter& first, const Iter& last) {
  size_t d = 0;
  Utf8Iterator<Iter> it(first);
  Utf8Iterator<Iter> end(utf8StringEnd(first, last));
  DCHECK(&end == last);

  for (; it != end; ++it)
    d += (*it > 0xFFFF) ? 2 : 1;

  return d;
}

} // namespace u16

///////////////////////////////////////////////////////////////////////////

std::string cutText(const std::string& text,
                    size_t start,
                    size_t prefixLen,
                    size_t len);

} // namespace rdd
