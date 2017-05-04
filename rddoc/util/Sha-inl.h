/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <array>
#include <openssl/ssl.h>
#include "rddoc/util/String.h"

namespace rdd {

template <class KeyString, class SaltString>
std::string sha256(const KeyString& key, const SaltString& salt) {
  std::array<uint8_t, SHA256_DIGEST_LENGTH> output;
  sha256((const uint8_t*)key.data(), key.size(),
         (const uint8_t*)salt.data(), salt.size(),
         output.data());
  return std::string((char*)output.data(), output.size());
}

template <class KeyString, class SaltString>
std::string sha256Hex(const KeyString& key, const SaltString& salt) {
  std::array<uint8_t, SHA256_DIGEST_LENGTH> output;
  sha256((const uint8_t*)key.data(), key.size(),
         (const uint8_t*)salt.data(), salt.size(),
         output.data());
  std::string hex;
  hexlify(output, hex);
  return hex;
}

}


