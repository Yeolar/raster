/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stddef.h>
#include "rddoc/util/Macro.h"

namespace rdd {

template <class InputString>
bool base64Encode(const InputString& input,
                  std::string& output,
                  Base64EncodeMode mode) {
  auto encode = [&](unsigned int i) -> char {
    static char kBase64EncTable[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    if (UNLIKELY(i == 0x3e)) return mode == Base64EncodeMode::URI ? '-' : '+';
    if (UNLIKELY(i == 0x3f)) return mode == Base64EncodeMode::URI ? '_' : '/';
    return kBase64EncTable[i];
  };

  static char kPad = '=';
  size_t j = 0;
  size_t n = input.size();
  output.resize((n + 2) / 3 * 4);
  unsigned int leftchar = 0;
  int leftbits = 0;
  for (size_t i = 0; i < n; ++i) {
    /* shift the data into buffer */
    leftchar = (leftchar << 8) | (unsigned char)input[i];
    leftbits += 8;
    /* see if there are 6-bit groups ready */
    while (leftbits >= 6) {
      leftbits -= 6;
      output[j++] = encode((leftchar >> leftbits) & 0x3f);
    }
  }
  if (leftbits == 2) {
    output[j++] = encode((leftchar & 0x3) << 4);
    output[j++] = kPad;
    output[j++] = kPad;
  }
  else if (leftbits == 4) {
    output[j++] = encode((leftchar & 0xf) << 2);
    output[j++] = kPad;
  }
  return true;
}

template <class InputString>
bool base64Decode(const InputString& input,
                  std::string& output,
                  Base64EncodeMode mode = Base64EncodeMode::COMMON) {
  auto decode = [](unsigned char c) -> char {
    // COMMON and URI mode compatible
    static char kBase64DecTable[] = {
      -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
      -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
      -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,62,-1,63,
      52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1, 0,-1,-1, /* Note PAD->0 */
      -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
      15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,63,
      -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
      41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
    };
    return kBase64DecTable[c];
  };

  static char kPad = '=';
  unsigned int value = 0;
  int bits = 0;
  size_t j = 0;
  size_t n = input.size();
  output.resize((n + 3) / 4 * 3);
  for (size_t i = 0; i < n; ++i) {
    unsigned char c = input[i];
    if (c > 0x7f) {
      continue;
    }
    if (c == kPad) {
      bits = 0;
      break;
    }
    c = decode(c);
    if (c == (unsigned char)-1) {
      continue;
    }
    /* shift it in on the low end, and see if there's
     * a byte ready for output. */
    value = (value << 6) | c;
    bits += 6;
    while (bits >= 8) {
      bits -= 8;
      output[j++] = (value >> bits) & 0xff;
    }
  }
  output.resize(j);
  return bits == 0;
}

} // namespace rdd
