/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <cstddef>
#include <string>
#include "rddoc/util/Range.h"

namespace rdd {

void sha256(const uint8_t* key, size_t keyLen,
            const uint8_t* salt, size_t saltLen,
            uint8_t* output);

template <class KeyString, class SaltString>
std::string sha256(const KeyString& key, const SaltString& salt);

template <class KeyString>
std::string sha256(const KeyString& key) {
  return sha256(key, ByteRange());
}

template <class KeyString, class SaltString>
std::string sha256Hex(const KeyString& key, const SaltString& salt);

template <class KeyString>
std::string sha256Hex(const KeyString& key) {
  return sha256Hex(key, ByteRange());
}

}

#include "Sha-inl.h"

