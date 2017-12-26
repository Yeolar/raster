/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

//#define U_HIDE_DRAFT_API 1

#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/normlzr.h>
#include <unicode/translit.h>

#include "raster/util/Unicode.h"
#include "raster/util/Utf8StringPiece.h"

namespace rdd {

std::string codePointToUtf8(char32_t cp) {
  std::string result;

  // Based on description from http://en.wikipedia.org/wiki/UTF-8.

  if (cp <= 0x7f) {
    result.resize(1);
    result[0] = static_cast<char>(cp);
  } else if (cp <= 0x7FF) {
    result.resize(2);
    result[1] = static_cast<char>(0x80 | (0x3f & cp));
    result[0] = static_cast<char>(0xC0 | (cp >> 6));
  } else if (cp <= 0xFFFF) {
    result.resize(3);
    result[2] = static_cast<char>(0x80 | (0x3f & cp));
    result[1] = (0x80 | static_cast<char>((0x3f & (cp >> 6))));
    result[0] = (0xE0 | static_cast<char>(cp >> 12));
  } else if (cp <= 0x10FFFF) {
    result.resize(4);
    result[3] = static_cast<char>(0x80 | (0x3f & cp));
    result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
    result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
    result[0] = static_cast<char>(0xF0 | (cp >> 18));
  }

  return result;
}

char32_t utf8ToCodePoint(const unsigned char*& p,
                         const unsigned char* const e,
                         bool skipOnError) {
  /* The following encodings are valid:
   *  7 U+0000  U+007F   1 0xxxxxxx
   * 11 U+0080  U+07FF   2 110xxxxx 10xxxxxx
   * 16 U+0800  U+FFFF   3 1110xxxx 10xxxxxx 10xxxxxx
   * 21 U+10000 U+10FFFF 4 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
   */

  auto skip = [&] { ++p; return U'\ufffd'; };

  if (p >= e) {
    if (skipOnError) return skip();
    throw std::runtime_error("rdd::utf8ToCodePoint empty/invalid string");
  }

  unsigned char fst = *p;
  if (!(fst & 0x80)) {
    // trivial case
    return *p++;
  }

  static const uint32_t bitMask[] = {
    (1 << 7) - 1,
    (1 << 11) - 1,
    (1 << 16) - 1,
    (1 << 21) - 1
  };

  // upper control bits are masked out later
  uint32_t d = fst;

  if ((fst & 0xC0) != 0xC0) {
    if (skipOnError) return skip();
    throw std::runtime_error(
      to<std::string>("rdd::utf8ToCodePoint i=0 d=", d));
  }

  fst <<= 1;

  for (unsigned int i = 1; i != 3 && p + i < e; ++i) {
    unsigned char tmp = p[i];

    if ((tmp & 0xC0) != 0x80) {
      if (skipOnError) return skip();
      throw std::runtime_error(
        to<std::string>("rdd::utf8ToCodePoint i=", i,
                        " tmp=", (uint32_t)tmp));
    }

    d = (d << 6) | (tmp & 0x3F);
    fst <<= 1;

    if (!(fst & 0x80)) {
      d &= bitMask[i];

      // overlong, could have been encoded with i bytes
      if ((d & ~bitMask[i - 1]) == 0) {
        if (skipOnError) return skip();
        throw std::runtime_error(
          to<std::string>("rdd::utf8ToCodePoint i=", i, " d=", d));
      }

      // check for surrogates only needed for 3 bytes
      if (i == 2) {
        if ((d >= 0xD800 && d <= 0xDFFF) || d > 0x10FFFF) {
          if (skipOnError) return skip();
          throw std::runtime_error(
            to<std::string>("rdd::utf8ToCodePoint i=", i, " d=", d));
        }
      }

      p += i + 1;
      return d;
    }
  }

  if (skipOnError) return skip();
  throw std::runtime_error("rdd::utf8ToCodePoint encoding length maxed out");
}

char32_t utf8ToCodePointUnsafe(const unsigned char* p) {
  char32_t cp = *p++;

  if (cp >= 0x80) {
    DCHECK(U8_IS_LEAD(cp));

    if (cp < 0xe0) {
      cp = ((cp & 0x1f) << 6) | (*p & 0x3f);
    } else if (cp < 0xf0) {
      /* no need for (c&0xf) because the upper bits are truncated
       * after <<12 in the cast to (char16_t) */
      cp = static_cast<char16_t>(((cp           ) << 12) |
                                 ((*p     & 0x3f) << 6) |
                                 ((*(p+1) & 0x3f)));
    } else {
      cp = (((cp     & 7   ) << 18) |
            ((*p     & 0x3f) << 12) |
            ((*(p+1) & 0x3f) << 6) |
            ((*(p+2) & 0x3f)));
    }
  }

  return cp;
}

namespace unicode {

// Reference:
//  https://en.wikipedia.org/wiki/Unicode_character_property
//  http://www.unicode.org/Public/5.1.0/ucd/UCD.html
//  ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt

/*
 * API access for C/POSIX character classes is as follows:
 * - alpha:     u_isUAlphabetic(c) or u_hasBinaryProperty(c, UCHAR_ALPHABETIC)
 * - lower:     u_isULowercase(c) or u_hasBinaryProperty(c, UCHAR_LOWERCASE)
 * - upper:     u_isUUppercase(c) or u_hasBinaryProperty(c, UCHAR_UPPERCASE)
 * - punct:     u_ispunct(c)
 * - digit:     u_isdigit(c) or u_charType(c)==U_DECIMAL_DIGIT_NUMBER
 * - xdigit:    u_isxdigit(c) or u_hasBinaryProperty(c, UCHAR_POSIX_XDIGIT)
 * - alnum:     u_hasBinaryProperty(c, UCHAR_POSIX_ALNUM)
 * - space:     u_isUWhiteSpace(c) or u_hasBinaryProperty(c, UCHAR_WHITE_SPACE)
 * - blank:     u_isblank(c) or u_hasBinaryProperty(c, UCHAR_POSIX_BLANK)
 * - cntrl:     u_charType(c)==U_CONTROL_CHAR
 * - graph:     u_hasBinaryProperty(c, UCHAR_POSIX_GRAPH)
 * - print:     u_hasBinaryProperty(c, UCHAR_POSIX_PRINT)
 *
 * Note: Some of the u_isxyz() functions in uchar.h predate, and do not match,
 * the Standard Recommendations in UTS #18. Instead, they match Java
 * functions according to their API documentation.
 *
 * Note: There are several ICU whitespace functions.
 * Comparison:
 * - u_isUWhiteSpace=UCHAR_WHITE_SPACE: Unicode White_Space property;
 *       most of general categories "Z" (separators) + most
 *       whitespace ISO controls
 *       (including no-break spaces, but excluding IS1..IS4 and ZWSP)
 * - u_isWhitespace: Java isWhitespace; Z + whitespace ISO controls
 *   but excluding no-break spaces
 * - u_isJavaSpaceChar: Java isSpaceChar; just Z (including no-break spaces)
 * - u_isspace: Z + whitespace ISO controls (including no-break spaces)
 * - u_isblank: "horizontal spaces" = TAB + Zs - ZWSP
 */

/**
 * Check if a code point has the Alphabetic Unicode property.
 * Same as u_hasBinaryProperty(c, UCHAR_ALPHABETIC).
 *
 * (JAVA) Determines whether the specified code point is a letter character.
 * True for general categories "L" (letters).
 */
bool isAlpha(char32_t cp) {
  if (cp < 0x80) return isalpha(cp);
#if RDD_UNICODE_JAVA_API
  return u_isalpha(cp);
#else
  return u_isUAlphabetic(cp);
#endif
}

/**
 * Check if a code point has the Lowercase Unicode property.
 * Same as u_hasBinaryProperty(c, UCHAR_LOWERCASE).
 *
 * (JAVA) Determines whether the specified code point has
 * the general category "Ll"
 * (lowercase letter).
 */
bool isLower(char32_t cp) {
  if (cp < 0x80) return islower(cp);
#if RDD_UNICODE_JAVA_API
  return u_islower(cp);
#else
  return u_isULowercase(cp);
#endif
}

/**
 * Check if a code point has the Uppercase Unicode property.
 * Same as u_hasBinaryProperty(c, UCHAR_UPPERCASE).
 *
 * (JAVA) Determines whether the specified code point has
 * the general category "Lu"
 * (uppercase letter).
 */
bool isUpper(char32_t cp) {
  if (cp < 0x80) return isupper(cp);
#if RDD_UNICODE_JAVA_API
  return u_isupper(cp);
#else
  return u_isUUppercase(cp);
#endif
}

/**
 * Determines whether the specified code point is a titlecase letter.
 * True for general category "Lt" (titlecase letter).
 */
bool isTitle(char32_t cp) {
  return u_istitle(cp);
}

/**
 * Determines whether the specified code point is a punctuation character.
 * True for characters with general categories "P" (punctuation).
 */
bool isPunct(char32_t cp) {
  if (cp < 0x80) return ispunct(cp);
  return u_ispunct(cp);
}

/**
 * Determines whether the specified code point is a digit character
 * according to Java.
 * True for characters with general category "Nd" (decimal digit numbers).
 * Beginning with Unicode 4, this is the same as
 * testing for the Numeric_Type of Decimal.
 */
bool isDigit(char32_t cp) {
  if (cp < 0x80) return isdigit(cp);
  return u_isdigit(cp);
}

/**
 * Determines whether the specified code point is a hexadecimal digit.
 * This is equivalent to u_digit(c, 16)>=0.
 * True for characters with general category "Nd" (decimal digit numbers)
 * as well as Latin letters a-f and A-F in both ASCII and Fullwidth ASCII.
 * (That is, for letters with code points
 * 0041..0046, 0061..0066, FF21..FF26, FF41..FF46.)
 *
 * In order to narrow the definition of hexadecimal digits to only ASCII
 * characters, use (c<=0x7f && u_isxdigit(c)).
 */
bool isXDigit(char32_t cp) {
  if (cp < 0x80) return isxdigit(cp);
  return u_isxdigit(cp);
}

/**
 * Check if a code point has the Alnum Unicode property.
 *
 * (JAVA) Determines whether the specified code point is
 * an alphanumeric character
 * (letter or digit) according to Java.
 * True for characters with general categories
 * "L" (letters) and "Nd" (decimal digit numbers).
 */
bool isAlnum(char32_t cp) {
  if (cp < 0x80) return isalnum(cp);
#if RDD_UNICODE_JAVA_API
  return u_isalnum(cp);
#else
  return u_hasBinaryProperty(cp, UCHAR_POSIX_ALNUM);
#endif
}

/**
 * Determines if the specified character is a space character or not.
 *
 * Note: There are several ICU whitespace functions; please see the uchar.h
 * file documentation for a detailed comparison.
 *
 * cp: 0009-000d 001c-0020 0085 00a0 1680 2000-200a 2028-2029 202f 205f
 *  = isWhitespace (C/POSIX) + 001c-001f
 */
bool isSpace(char32_t cp) {
  return u_isspace(cp);
}

/**
 * Determine if the specified code point is a space character according to Java.
 * True for characters with general categories "Z" (separators),
 * which does not include control codes (e.g., TAB or Line Feed).
 *
 * Note: There are several ICU whitespace functions; please see the uchar.h
 * file documentation for a detailed comparison.
 *
 * cp: 0020 00a0 1680 2000-200a 2028-2029 202f 205f
 *  = isWhitespace (C/POSIX) - 0009-000d - 0085
 */
bool isJavaSpace(char32_t cp) {
  return u_isJavaSpaceChar(cp);
}

/**
 * Check if a code point has the White_Space Unicode property.
 * Same as u_hasBinaryProperty(c, UCHAR_WHITE_SPACE).
 *
 * (JAVA) Determines if the specified code point is a whitespace character
 * according to Java/ICU.
 * A character is considered to be a Java whitespace character if and only
 * if it satisfies one of the following criteria:
 *
 * - It is a Unicode Separator character (categories "Z" = "Zs" or "Zl"
 *   or "Zp"), but is not
 *      also a non-breaking space (U+00A0 NBSP or U+2007 Figure Space
 *      or U+202F Narrow NBSP).
 * - It is U+0009 HORIZONTAL TABULATION.
 * - It is U+000A LINE FEED.
 * - It is U+000B VERTICAL TABULATION.
 * - It is U+000C FORM FEED.
 * - It is U+000D CARRIAGE RETURN.
 * - It is U+001C FILE SEPARATOR.
 * - It is U+001D GROUP SEPARATOR.
 * - It is U+001E RECORD SEPARATOR.
 * - It is U+001F UNIT SEPARATOR.
 *
 * Note: Unicode 4.0.1 changed U+200B ZERO WIDTH SPACE from
 * a Space Separator (Zs)
 * to a Format Control (Cf). Since then, isWhitespace(0x200b) returns false.
 * See http://www.unicode.org/versions/Unicode4.0.1/
 *
 * Note: There are several ICU whitespace functions; please see the uchar.h
 * file documentation for a detailed comparison.
 *
 * cp: 0009-000d 001c-0020 1680 2000-2006 2008-200a 2028-2029 205f (JAVA)
 *
 * cp: 0009-000d 0020 0085 00a0 1680 2000-200a 2028-2029 202f 205f
 *  = isBlank + 000a-000d + 0085 + 2028-2029
 *  = kCFCharacterSetWhitespaceAndNewline - 200b
 */
bool isWhitespace(char32_t cp) {
#if RDD_UNICODE_JAVA_API
  return u_isWhitespace(cp);
#else
  return u_isUWhiteSpace(cp);
#endif
}

/**
 * Determines whether the specified code point is a "blank" or
 * "horizontal space",
 * a character that visibly separates words on a line.
 * The following are equivalent definitions:
 *
 * TRUE for Unicode White_Space characters except for "vertical space controls"
 * where "vertical space controls" are the following characters:
 * U+000A (LF) U+000B (VT) U+000C (FF) U+000D (CR) U+0085 (NEL) U+2028 (LS)
 * U+2029 (PS)
 *
 * same as
 *
 * TRUE for U+0009 (TAB) and characters with general category "Zs"
 * (space separators)
 * except Zero Width Space (ZWSP, U+200B).
 *
 * Note: There are several ICU whitespace functions; please see the uchar.h
 * file documentation for a detailed comparison.
 *
 * cp: 0009 0020 00a0 1680 2000-200a 202f 205f
 *  = kCFCharacterSetWhitespace - 200b
 */
bool isBlank(char32_t cp) {
  if (cp < 0x80) return isblank(cp);
  return u_isblank(cp);
}

/**
 * Check if a code point is a control char.
 *
 * (JAVA) Determines whether the specified code point is a control character
 * (as defined by this function).
 * A control character is one of the following:
 * - ISO 8-bit control character (U+0000..U+001f and U+007f..U+009f)
 * - U_CONTROL_CHAR (Cc)
 * - U_FORMAT_CHAR (Cf)
 * - U_LINE_SEPARATOR (Zl)
 * - U_PARAGRAPH_SEPARATOR (Zp)
 */
bool isCntrl(char32_t cp) {
  if (cp < 0x80) return iscntrl(cp);
#if RDD_UNICODE_JAVA_API
  return u_iscntrl(cp);
#else
  return u_charType(cp)==U_CONTROL_CHAR;
#endif
}

/**
 * Determines whether the specified code point is an ISO control code.
 * True for U+0000..U+001f and U+007f..U+009f (general category "Cc").
 */
bool isISOControl(char32_t cp) {
  if (cp < 0x80) return iscntrl(cp);
  return u_isISOControl(cp);
}

/**
 * Check if a code point has the Graph Unicode property.
 *
 * (JAVA) Determines whether the specified code point is a "graphic" character
 * (printable, excluding spaces).
 * TRUE for all characters except those with general categories
 * "Cc" (control codes), "Cf" (format controls), "Cs" (surrogates),
 * "Cn" (unassigned), and "Z" (separators).
 */
bool isGraph(char32_t cp) {
  if (cp < 0x80) return isgraph(cp);
#if RDD_UNICODE_JAVA_API
  return u_isgraph(cp);
#else
  return u_hasBinaryProperty(cp, UCHAR_POSIX_GRAPH);
#endif
}

/**
 * Check if a code point has the Print Unicode property.
 *
 * (JAVA) Determines whether the specified code point is a printable character.
 * True for general categories <em>other</em> than "C" (controls).
 */
bool isPrint(char32_t cp) {
  if (cp < 0x80) return isprint(cp);
#if RDD_UNICODE_JAVA_API
  return u_isprint(cp);
#else
  return u_hasBinaryProperty(cp, UCHAR_POSIX_PRINT);
#endif
}

/**
 * Determines whether the specified code point is "defined",
 * which usually means that it is assigned a character.
 * True for general categories other than "Cn" (other, not assigned),
 * i.e., true for all code points mentioned in UnicodeData.txt.
 *
 * Note that non-character code points (e.g., U+FDD0) are not "defined"
 * (they are Cn), but surrogate code points are "defined" (Cs).
 */
bool isDefined(char32_t cp) {
  return u_isdefined(cp);
}

/**
 * Determines whether the specified code point is a base character.
 * True for general categories "L" (letters), "N" (numbers),
 * "Mc" (spacing combining marks), and "Me" (enclosing marks).
 *
 * Note that this is different from the Unicode definition in
 * chapter 3.5, conformance clause D13,
 * which defines base characters to be all characters (not Cn)
 * that do not graphically combine with preceding characters (M)
 * and that are neither control (Cc) or format (Cf) characters.
 */
bool isBase(char32_t cp) {
  return u_isbase(cp);
}

/**
 * Returns the general category value for the code point.
 * Value defined in enum UCharCategory.
 */
int8_t getUCharType(char32_t cp) {
  return u_charType(cp);
}

/**
 * Get a single-bit bit set for the general category of a character.
 * This bit set can be compared bitwise with U_GC_SM_MASK, U_GC_L_MASK, etc.
 * Same as U_MASK(u_charType(c)).
 */
uint32_t getUCharCategoryMask(char32_t cp) {
  return U_GET_GC_MASK(cp);
}

bool isMemberOfUCharCategory(char32_t cp, uint32_t mask) {
  return U_GET_GC_MASK(cp) & mask;
}

// = kCFCharacterSetNonBase (ignore unassigned code point).
bool isNonBase(char32_t cp) {
  return U_GET_GC_MASK(cp) & U_GC_M_MASK;
}

// East_Asian_Width Property:
//  http://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt
// Original also include (?):
//  0x11A3, 0x11A4, 0x11A5, 0x11A6, 0x11A7, 0x11FA, 0x11FB, 0x11FC, 0x11FD,
//  0x11FE, 0x11FF
//
// Other implementation see:
//  Python/Modules/unicodedata.c
//
bool isWide(char32_t cp) {
  UEastAsianWidth ea =
  (UEastAsianWidth)u_getIntPropertyValue(cp, UCHAR_EAST_ASIAN_WIDTH);
  return ea == U_EA_FULLWIDTH || ea == U_EA_WIDE;
}

/**
 * The given character is mapped to its lowercase equivalent according to
 * UnicodeData.txt; if the character has no lowercase equivalent, the character
 * itself is returned.
 *
 * This function only returns the simple, single-code point case mapping.
 * Full case mappings should be used whenever possible because they produce
 * better results by working on whole strings.
 * They take into account the string context and the language and can map
 * to a result string with a different length as appropriate.
 * Full case mappings are applied by the string case mapping functions,
 * see ustring.h and the UnicodeString class.
 * See also the User Guide chapter on C/POSIX migration:
 * http://icu-project.org/userguide/posix.html#case_mappings
 */
char32_t toLower(char32_t cp) {
  return u_tolower(cp);
}

/**
 * The given character is mapped to its uppercase equivalent
 * according to UnicodeData.txt;
 * if the character has no uppercase equivalent, the character itself is
 * returned.
 *
 * This function only returns the simple, single-code point case mapping.
 * Full case mappings should be used whenever possible because they produce
 * better results by working on whole strings.
 * They take into account the string context and the language and can map
 * to a result string with a different length as appropriate.
 * Full case mappings are applied by the string case mapping functions,
 * see ustring.h and the UnicodeString class.
 * See also the User Guide chapter on C/POSIX migration:
 * http://icu-project.org/userguide/posix.html#case_mappings
 */
char32_t toUpper(char32_t cp) {
  return u_toupper(cp);
}

std::string toLower(const rdd::StringPiece& sp) {
  std::string result;
  icu::StringPiece usp(sp.begin(), (int32_t)sp.size());
  icu::UnicodeString::fromUTF8(usp).toLower().toUTF8String(result);
  return result;
}

std::string toUpper(const rdd::StringPiece& sp) {
  std::string result;
  icu::StringPiece usp(sp.begin(), (int32_t)sp.size());
  icu::UnicodeString::fromUTF8(usp).toUpper().toUTF8String(result);
  return result;
}

std::string toOppositeCase(const rdd::StringPiece& sp) {
  std::string result;
  Utf8StringPiece usp(sp);
  size_t curr, pos = 0;
  int cpCase = -1;

  for (auto it = usp.begin(); it != usp.end(); ++it) {
    if (isLower(*it)) {
      if (cpCase == 2) {
        curr = &it - &usp.begin();
        result.append(toLower(sp.subpiece(pos, curr - pos)));
        pos = curr;
      }
      cpCase = 1;
    } else if (isUpper(*it) || isTitle(*it)) {
      if (cpCase == 1) {
        curr = &it - &usp.begin();
        result.append(toUpper(sp.subpiece(pos, curr - pos)));
        pos = curr;
      }
      cpCase = 2;
    }
  }
  if (cpCase == 1) {
    result.append(toUpper(sp.subpiece(pos)));
  }
  if (cpCase == 2) {
    result.append(toLower(sp.subpiece(pos)));
  }

  return result;
}

std::string normalize(const rdd::StringPiece& sp, NormalizationMode mode) {
  std::string result;
  icu::StringPiece usp(sp.begin(), (int32_t)sp.size());
  icu::UnicodeString us = icu::UnicodeString::fromUTF8(usp);
  icu::UnicodeString out;
  UErrorCode err = U_ZERO_ERROR;
  icu::Normalizer::normalize(us, (UNormalizationMode)mode, 0, out, err);
  if (err != U_ZERO_ERROR) {
    RDDLOG(WARN) << "normalize error: " << err;
  }
  out.toUTF8String(result);
  return result;
}

std::string asciify(const rdd::StringPiece& sp) {
  std::string result;
  icu::StringPiece usp(sp.begin(), (int32_t)sp.size());
  icu::UnicodeString us = icu::UnicodeString::fromUTF8(usp);
  UErrorCode err = U_ZERO_ERROR;
  icu::Transliterator* trans =
  icu::Transliterator::createInstance(
      "any-Latin; Latin-ASCII", UTRANS_FORWARD, err);
  if (err != U_ZERO_ERROR) {
    RDDLOG(WARN) << "Create Transliterator instance error: " << err;
  }
  trans->transliterate(us);
  us.toUTF8String(result);
  return result;
}

} // namespace unicode

std::string cutText(const std::string& text,
                    size_t start,
                    size_t prefixLen,
                    size_t len) {
  auto isContiGraph = [](char32_t c1, char32_t c2) {
    return (c1 < 0x80 && isgraph(c1)) && (c2 < 0x80 && isgraph(c2));
  };
  auto b = makeUtf8Iterator(text.data());
  auto e = makeUtf8Iterator(text.data() + text.size());
  auto p = makeUtf8Iterator(text.data() + start);
  size_t i = 0;
  if (p != b) {
    while (--p != b) {
      if (i >= prefixLen && !isContiGraph(*p, *(p+1))) { p++; break; }
      if (unicode::isWide(*p)) i++; i++;
    }
  }
  auto q = p;
  i = 0;
  while (q != e) {
    if (i >= len && !isContiGraph('_', *q)) break;
    if (unicode::isWide(*q)) i++; i++; q++;
  }
  return to<std::string>((p != b ? "..." : ""),
                         utf8Piece(p, q).str(),
                         (q != e ? "..." : ""));
}

} // namespace rdd
