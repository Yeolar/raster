/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/String.h"
#include "rddoc/util/ScopeGuard.h"

#include <cstdarg>
#include <cstring>
#include <array>
#include <memory>
#include <stdexcept>

namespace rdd {

namespace {

int stringAppendfImplHelper(char* buf, size_t bufsize, const char* format,
                            va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  int bytes_used = vsnprintf(buf, bufsize, format, args_copy);
  va_end(args_copy);
  return bytes_used;
}

void stringAppendfImpl(std::string& output, const char* format, va_list args) {
  // Very simple; first, try to avoid an allocation by using an inline
  // buffer.  If that fails to hold the output string, allocate one on
  // the heap, use it instead.
  //
  // It is hard to guess the proper size of this buffer; some
  // heuristics could be based on the number of format characters, or
  // static analysis of a codebase.  Or, we can just pick a number
  // that seems big enough for simple cases (say, one line of text on
  // a terminal) without being large enough to be concerning as a
  // stack variable.
  std::array<char, 128> inline_buffer;

  int bytes_used = stringAppendfImplHelper(
    inline_buffer.data(), inline_buffer.size(), format, args);
  if (bytes_used < 0) {
    throw std::runtime_error(
      std::string(
        "Invalid format string; snprintf returned negative "
        "with format string: ") + format);
  }

  if (static_cast<size_t>(bytes_used) < inline_buffer.size()) {
    output.append(inline_buffer.data(), bytes_used);
    return;
  }

  // Couldn't fit.  Heap allocate a buffer, oh well.
  std::unique_ptr<char[]> heap_buffer(new char[bytes_used + 1]);
  int final_bytes_used = stringAppendfImplHelper(
    heap_buffer.get(), bytes_used + 1, format, args);
  // The second call can take fewer bytes if, for example, we were printing a
  // string buffer with null-terminating char using a width specifier -
  // vsnprintf("%.*s", buf.size(), buf)
  assert(bytes_used >= final_bytes_used);

  // We don't keep the trailing '\0' in our output string
  output.append(heap_buffer.get(), final_bytes_used);
}

} // namespace

std::string stringPrintf(const char* format, ...) {
  std::string ret;
  va_list ap;
  va_start(ap, format);
  stringAppendfImpl(ret, format, ap);
  va_end(ap);
  return ret;
}

std::string stringVPrintf(const char* format, va_list ap) {
  std::string ret;
  stringAppendfImpl(ret, format, ap);
  return ret;
}

// Basic declarations; allow for parameters of strings and string
// pieces to be specified.
std::string& stringAppendf(std::string* output, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  stringAppendfImpl(*output, format, ap);
  va_end(ap);
  return *output;
}

std::string& stringVAppendf(std::string* output, const char* format,
                            va_list ap) {
  stringAppendfImpl(*output, format, ap);
  return *output;
}

void stringPrintf(std::string* output, const char* format, ...) {
  output->clear();
  va_list ap;
  va_start(ap, format);
  stringAppendfImpl(*output, format, ap);
  va_end(ap);
}

void stringVPrintf(std::string* output, const char* format, va_list ap) {
  output->clear();
  stringAppendfImpl(*output, format, ap);
};

std::string errnoStr(int err) {
  int savedErrno = errno;

  // Ensure that we reset errno upon exit.
  auto guard(makeGuard([&] { errno = savedErrno; }));

  char buf[1024];
  buf[0] = '\0';

  std::string result;

  // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/strerror_r.3.html
  // http://www.kernel.org/doc/man-pages/online/pages/man3/strerror.3.html
#if defined(RDD_HAVE_XSI_STRERROR_R) || \
  defined(__APPLE__) || defined(__ANDROID__)
  // Using XSI-compatible strerror_r
  int r = strerror_r(err, buf, sizeof(buf));

  // OSX/FreeBSD use EINVAL and Linux uses -1 so just check for non-zero
  if (r != 0) {
    result = to<std::string>(
      "Unknown error ", err, " (strerror_r failed with error ", errno, ")");
  } else {
    result.assign(buf);
  }
#else
  // Using GNU strerror_r
  result.assign(strerror_r(err, buf, sizeof(buf)));
#endif

  return result;
}

static inline bool is_oddspace(char c) {
  return c == '\n' || c == '\t' || c == '\r';
}

StringPiece ltrimWhitespace(StringPiece sp) {
// Spaces other than ' ' characters are less common but should be
// checked.  This configuration where we loop on the ' '
// separately from oddspaces was empirically fastest.

loop:
  for (; !sp.empty() && sp.front() == ' '; sp.pop_front()) {
  }
  if (!sp.empty() && is_oddspace(sp.front())) {
    sp.pop_front();
    goto loop;
  }

  return sp;
}

StringPiece rtrimWhitespace(StringPiece sp) {
// Spaces other than ' ' characters are less common but should be
// checked.  This configuration where we loop on the ' '
// separately from oddspaces was empirically fastest.

loop:
  for (; !sp.empty() && sp.back() == ' '; sp.pop_back()) {
  }
  if (!sp.empty() && is_oddspace(sp.back())) {
    sp.pop_back();
    goto loop;
  }

  return sp;
}

namespace {

void toLowerAscii8(char& c) {
  // Branchless tolower, based on the input-rotating trick described
  // at http://www.azillionmonkeys.com/qed/asmexample.html
  //
  // This algorithm depends on an observation: each uppercase
  // ASCII character can be converted to its lowercase equivalent
  // by adding 0x20.

  // Step 1: Clear the high order bit. We'll deal with it in Step 5.
  unsigned char rotated = c & 0x7f;
  // Currently, the value of rotated, as a function of the original c is:
  //   below 'A':   0- 64
  //   'A'-'Z':    65- 90
  //   above 'Z':  91-127

  // Step 2: Add 0x25 (37)
  rotated += 0x25;
  // Now the value of rotated, as a function of the original c is:
  //   below 'A':   37-101
  //   'A'-'Z':    102-127
  //   above 'Z':  128-164

  // Step 3: clear the high order bit
  rotated &= 0x7f;
  //   below 'A':   37-101
  //   'A'-'Z':    102-127
  //   above 'Z':    0- 36

  // Step 4: Add 0x1a (26)
  rotated += 0x1a;
  //   below 'A':   63-127
  //   'A'-'Z':    128-153
  //   above 'Z':   25- 62

  // At this point, note that only the uppercase letters have been
  // transformed into values with the high order bit set (128 and above).

  // Step 5: Shift the high order bit 2 spaces to the right: the spot
  // where the only 1 bit in 0x20 is.  But first, how we ignored the
  // high order bit of the original c in step 1?  If that bit was set,
  // we may have just gotten a false match on a value in the range
  // 128+'A' to 128+'Z'.  To correct this, need to clear the high order
  // bit of rotated if the high order bit of c is set.  Since we don't
  // care about the other bits in rotated, the easiest thing to do
  // is invert all the bits in c and bitwise-and them with rotated.
  rotated &= ~c;
  rotated >>= 2;

  // Step 6: Apply a mask to clear everything except the 0x20 bit
  // in rotated.
  rotated &= 0x20;

  // At this point, rotated is 0x20 if c is 'A'-'Z' and 0x00 otherwise

  // Step 7: Add rotated to c
  c += rotated;
}

void toLowerAscii32(uint32_t& c) {
  // Besides being branchless, the algorithm in toLowerAscii8() has another
  // interesting property: None of the addition operations will cause
  // an overflow in the 8-bit value.  So we can pack four 8-bit values
  // into a uint32_t and run each operation on all four values in parallel
  // without having to use any CPU-specific SIMD instructions.
  uint32_t rotated = c & uint32_t(0x7f7f7f7fL);
  rotated += uint32_t(0x25252525L);
  rotated &= uint32_t(0x7f7f7f7fL);
  rotated += uint32_t(0x1a1a1a1aL);

  // Step 5 involves a shift, so some bits will spill over from each
  // 8-bit value into the next.  But that's okay, because they're bits
  // that will be cleared by the mask in step 6 anyway.
  rotated &= ~c;
  rotated >>= 2;
  rotated &= uint32_t(0x20202020L);
  c += rotated;
}

void toLowerAscii64(uint64_t& c) {
  // 64-bit version of toLower32
  uint64_t rotated = c & uint64_t(0x7f7f7f7f7f7f7f7fL);
  rotated += uint64_t(0x2525252525252525L);
  rotated &= uint64_t(0x7f7f7f7f7f7f7f7fL);
  rotated += uint64_t(0x1a1a1a1a1a1a1a1aL);
  rotated &= ~c;
  rotated >>= 2;
  rotated &= uint64_t(0x2020202020202020L);
  c += rotated;
}

} // namespace

void toLowerAscii(char* str, size_t length) {
  static const size_t kAlignMask64 = 7;
  static const size_t kAlignMask32 = 3;

  // Convert a character at a time until we reach an address that
  // is at least 32-bit aligned
  size_t n = (size_t)str;
  n &= kAlignMask32;
  n = std::min(n, length);
  size_t offset = 0;
  if (n != 0) {
    n = std::min(4 - n, length);
    do {
      toLowerAscii8(str[offset]);
      offset++;
    } while (offset < n);
  }

  n = (size_t)(str + offset);
  n &= kAlignMask64;
  if ((n != 0) && (offset + 4 <= length)) {
    // The next address is 32-bit aligned but not 64-bit aligned.
    // Convert the next 4 bytes in order to get to the 64-bit aligned
    // part of the input.
    toLowerAscii32(*(uint32_t*)(str + offset));
    offset += 4;
  }

  // Convert 8 characters at a time
  while (offset + 8 <= length) {
    toLowerAscii64(*(uint64_t*)(str + offset));
    offset += 8;
  }

  // Convert 4 characters at a time
  while (offset + 4 <= length) {
    toLowerAscii32(*(uint32_t*)(str + offset));
    offset += 4;
  }

  // Convert any characters remaining after the last 4-byte aligned group
  while (offset < length) {
    toLowerAscii8(str[offset]);
    offset++;
  }
}

} // namespace rdd
