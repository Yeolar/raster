/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <openssl/ssl.h>
#include "raster/ssl/OpenSSL.h"
#include "raster/util/Logging.h"
#include "raster/util/Memory.h"

namespace rdd {

// ASN1
using ASN1TimeDeleter =
    rdd::static_function_deleter<ASN1_TIME, &ASN1_TIME_free>;
using ASN1TimeUniquePtr = std::unique_ptr<ASN1_TIME, ASN1TimeDeleter>;

// X509
using X509Deleter = rdd::static_function_deleter<X509, &X509_free>;
using X509UniquePtr = std::unique_ptr<X509, X509Deleter>;
using X509StoreCtxDeleter =
    rdd::static_function_deleter<X509_STORE_CTX, &X509_STORE_CTX_free>;
using X509StoreCtxUniquePtr =
    std::unique_ptr<X509_STORE_CTX, X509StoreCtxDeleter>;
using X509VerifyParamDeleter =
    rdd::static_function_deleter<X509_VERIFY_PARAM, &X509_VERIFY_PARAM_free>;
using X509VerifyParam =
    std::unique_ptr<X509_VERIFY_PARAM, X509VerifyParamDeleter>;

// EVP
using EvpPkeyDel = rdd::static_function_deleter<EVP_PKEY, &EVP_PKEY_free>;
using EvpPkeyUniquePtr = std::unique_ptr<EVP_PKEY, EvpPkeyDel>;
using EvpPkeySharedPtr = std::shared_ptr<EVP_PKEY>;

// No EVP_PKEY_CTX <= 0.9.8b
#if OPENSSL_VERSION_NUMBER >= 0x10000002L
using EvpPkeyCtxDeleter =
    rdd::static_function_deleter<EVP_PKEY_CTX, &EVP_PKEY_CTX_free>;
using EvpPkeyCtxUniquePtr = std::unique_ptr<EVP_PKEY_CTX, EvpPkeyCtxDeleter>;
#else
struct EVP_PKEY_CTX;
#endif

using EvpMdCtxDeleter =
    rdd::static_function_deleter<EVP_MD_CTX, &EVP_MD_CTX_free>;
using EvpMdCtxUniquePtr = std::unique_ptr<EVP_MD_CTX, EvpMdCtxDeleter>;

// HMAC
using HmacCtxDeleter = rdd::static_function_deleter<HMAC_CTX, &HMAC_CTX_free>;
using HmacCtxUniquePtr = std::unique_ptr<HMAC_CTX, HmacCtxDeleter>;

// BIO
using BioMethodDeleter =
    rdd::static_function_deleter<BIO_METHOD, &BIO_meth_free>;
using BioMethodUniquePtr = std::unique_ptr<BIO_METHOD, BioMethodDeleter>;
using BioDeleter = rdd::static_function_deleter<BIO, &BIO_vfree>;
using BioUniquePtr = std::unique_ptr<BIO, BioDeleter>;
using BioChainDeleter = rdd::static_function_deleter<BIO, &BIO_free_all>;
using BioChainUniquePtr = std::unique_ptr<BIO, BioChainDeleter>;
inline void BIO_free_fb(BIO* bio) { RDDCHECK_EQ(1, BIO_free(bio)); }
using BioDeleterFb = rdd::static_function_deleter<BIO, &BIO_free_fb>;
using BioUniquePtrFb = std::unique_ptr<BIO, BioDeleterFb>;

// RSA and EC
using RsaDeleter = rdd::static_function_deleter<RSA, &RSA_free>;
using RsaUniquePtr = std::unique_ptr<RSA, RsaDeleter>;
#ifndef OPENSSL_NO_EC
using EcKeyDeleter = rdd::static_function_deleter<EC_KEY, &EC_KEY_free>;
using EcKeyUniquePtr = std::unique_ptr<EC_KEY, EcKeyDeleter>;
using EcGroupDeleter = rdd::static_function_deleter<EC_GROUP, &EC_GROUP_free>;
using EcGroupUniquePtr = std::unique_ptr<EC_GROUP, EcGroupDeleter>;
using EcPointDeleter = rdd::static_function_deleter<EC_POINT, &EC_POINT_free>;
using EcPointUniquePtr = std::unique_ptr<EC_POINT, EcPointDeleter>;
using EcdsaSignDeleter =
    rdd::static_function_deleter<ECDSA_SIG, &ECDSA_SIG_free>;
using EcdsaSigUniquePtr = std::unique_ptr<ECDSA_SIG, EcdsaSignDeleter>;
#endif

// BIGNUMs
using BIGNUMDeleter = rdd::static_function_deleter<BIGNUM, &BN_clear_free>;
using BIGNUMUniquePtr = std::unique_ptr<BIGNUM, BIGNUMDeleter>;

// SSL and SSL_CTX
using SSLDeleter = rdd::static_function_deleter<SSL, &SSL_free>;
using SSLUniquePtr = std::unique_ptr<SSL, SSLDeleter>;

} // namespace rdd
