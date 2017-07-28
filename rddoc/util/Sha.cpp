/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/util/Sha.h"
#include <openssl/ssl.h>

namespace rdd {

void sha256(const uint8_t* key, size_t keyLen,
            const uint8_t* salt, size_t saltLen,
            uint8_t* output) {
  SHA256_CTX hash_ctx;

  SHA256_Init(&hash_ctx);
  SHA256_Update(&hash_ctx, key, keyLen);
  SHA256_Update(&hash_ctx, salt, saltLen);
  SHA256_Final(output, &hash_ctx);
}

} // namespace rdd
