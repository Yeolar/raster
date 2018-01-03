/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#include "raster/util/Checksum.h"

#include <stdexcept>
#include <immintrin.h>
#include <nmmintrin.h>
#include <boost/crc.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

namespace rdd {

namespace detail {

template <uint32_t CRC_POLYNOMIAL>
uint32_t crc_sw(const uint8_t* data, size_t nbytes, uint32_t startingChecksum) {
  // Reverse the bits in the starting checksum so they'll be in the
  // right internal format for Boost's CRC engine.
  //     O(1)-time, branchless bit reversal algorithm from
  //     http://graphics.stanford.edu/~seander/bithacks.html
  startingChecksum = ((startingChecksum >> 1) & 0x55555555) |
      ((startingChecksum & 0x55555555) << 1);
  startingChecksum = ((startingChecksum >> 2) & 0x33333333) |
      ((startingChecksum & 0x33333333) << 2);
  startingChecksum = ((startingChecksum >> 4) & 0x0f0f0f0f) |
      ((startingChecksum & 0x0f0f0f0f) << 4);
  startingChecksum = ((startingChecksum >> 8) & 0x00ff00ff) |
      ((startingChecksum & 0x00ff00ff) << 8);
  startingChecksum = (startingChecksum >> 16) |
      (startingChecksum << 16);

  boost::crc_optimal<32, CRC_POLYNOMIAL, ~0U, 0, true, true> sum(
      startingChecksum);
  sum.process_bytes(data, nbytes);
  return sum.checksum();
}

uint32_t
crc32c_sw(const uint8_t* data, size_t nbytes, uint32_t startingChecksum) {
  constexpr uint32_t CRC32C_POLYNOMIAL = 0x1EDC6F41;
  return crc_sw<CRC32C_POLYNOMIAL>(data, nbytes, startingChecksum);
}

uint32_t
crc32_sw(const uint8_t* data, size_t nbytes, uint32_t startingChecksum) {
  constexpr uint32_t CRC32_POLYNOMIAL = 0x04C11DB7;
  return crc_sw<CRC32_POLYNOMIAL>(data, nbytes, startingChecksum);
}

/*
 * Copyright 2016 Ferry Toth, Exalon Delft BV, The Netherlands
 *
 * https://github.com/htot/crc32c
 *
 * Original intel whitepaper:
 * "Fast CRC Computation for iSCSI Polynomial Using CRC32 Instruction"
 * https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/crc-iscsi-polynomial-crc32-instruction-paper.pdf
 *
 * 32-bit support dropped
 * use intrinsics instead of inline asm
 * other code cleanup
 */

namespace crc32_detail {

#define CRCtriplet(crc, buf, offset)                  \
  crc##0 = _mm_crc32_u64(crc##0, *(buf##0 + offset)); \
  crc##1 = _mm_crc32_u64(crc##1, *(buf##1 + offset)); \
  crc##2 = _mm_crc32_u64(crc##2, *(buf##2 + offset));

#define CRCduplet(crc, buf, offset)                   \
  crc##0 = _mm_crc32_u64(crc##0, *(buf##0 + offset)); \
  crc##1 = _mm_crc32_u64(crc##1, *(buf##1 + offset));

#define CRCsinglet(crc, buf, offset)                  \
  crc = _mm_crc32_u64(crc, *(uint64_t*)(buf + offset));

#define CASEREPEAT_TRIPLET(unused, count, total)      \
  case BOOST_PP_ADD(1, BOOST_PP_SUB(total, count)):   \
    CRCtriplet(crc, next, -BOOST_PP_ADD(1, BOOST_PP_SUB(total, count)));

#define CASEREPEAT_SINGLET(unused, count, total)      \
  case BOOST_PP_SUB(total, count):                    \
    CRCsinglet(crc0, next, -BOOST_PP_SUB(total, count) * 8);

// Numbers taken directly from intel whitepaper.
const __m128i clmul_constants[] = {
    {0x14cd00bd6, 0x105ec76f0}, {0x0ba4fc28e, 0x14cd00bd6},
    {0x1d82c63da, 0x0f20c0dfe}, {0x09e4addf8, 0x0ba4fc28e},
    {0x039d3b296, 0x1384aa63a}, {0x102f9b8a2, 0x1d82c63da},
    {0x14237f5e6, 0x01c291d04}, {0x00d3b6092, 0x09e4addf8},
    {0x0c96cfdc0, 0x0740eef02}, {0x18266e456, 0x039d3b296},
    {0x0daece73e, 0x0083a6eec}, {0x0ab7aff2a, 0x102f9b8a2},
    {0x1248ea574, 0x1c1733996}, {0x083348832, 0x14237f5e6},
    {0x12c743124, 0x02ad91c30}, {0x0b9e02b86, 0x00d3b6092},
    {0x018b33a4e, 0x06992cea2}, {0x1b331e26a, 0x0c96cfdc0},
    {0x17d35ba46, 0x07e908048}, {0x1bf2e8b8a, 0x18266e456},
    {0x1a3e0968a, 0x11ed1f9d8}, {0x0ce7f39f4, 0x0daece73e},
    {0x061d82e56, 0x0f1d0f55e}, {0x0d270f1a2, 0x0ab7aff2a},
    {0x1c3f5f66c, 0x0a87ab8a8}, {0x12ed0daac, 0x1248ea574},
    {0x065863b64, 0x08462d800}, {0x11eef4f8e, 0x083348832},
    {0x1ee54f54c, 0x071d111a8}, {0x0b3e32c28, 0x12c743124},
    {0x0064f7f26, 0x0ffd852c6}, {0x0dd7e3b0c, 0x0b9e02b86},
    {0x0f285651c, 0x0dcb17aa4}, {0x010746f3c, 0x018b33a4e},
    {0x1c24afea4, 0x0f37c5aee}, {0x0271d9844, 0x1b331e26a},
    {0x08e766a0c, 0x06051d5a2}, {0x093a5f730, 0x17d35ba46},
    {0x06cb08e5c, 0x11d5ca20e}, {0x06b749fb2, 0x1bf2e8b8a},
    {0x1167f94f2, 0x021f3d99c}, {0x0cec3662e, 0x1a3e0968a},
    {0x19329634a, 0x08f158014}, {0x0e6fc4e6a, 0x0ce7f39f4},
    {0x08227bb8a, 0x1a5e82106}, {0x0b0cd4768, 0x061d82e56},
    {0x13c2b89c4, 0x188815ab2}, {0x0d7a4825c, 0x0d270f1a2},
    {0x10f5ff2ba, 0x105405f3e}, {0x00167d312, 0x1c3f5f66c},
    {0x0f6076544, 0x0e9adf796}, {0x026f6a60a, 0x12ed0daac},
    {0x1a2adb74e, 0x096638b34}, {0x19d34af3a, 0x065863b64},
    {0x049c3cc9c, 0x1e50585a0}, {0x068bce87a, 0x11eef4f8e},
    {0x1524fa6c6, 0x19f1c69dc}, {0x16cba8aca, 0x1ee54f54c},
    {0x042d98888, 0x12913343e}, {0x1329d9f7e, 0x0b3e32c28},
    {0x1b1c69528, 0x088f25a3a}, {0x02178513a, 0x0064f7f26},
    {0x0e0ac139e, 0x04e36f0b0}, {0x0170076fa, 0x0dd7e3b0c},
    {0x141a1a2e2, 0x0bd6f81f8}, {0x16ad828b4, 0x0f285651c},
    {0x041d17b64, 0x19425cbba}, {0x1fae1cc66, 0x010746f3c},
    {0x1a75b4b00, 0x18db37e8a}, {0x0f872e54c, 0x1c24afea4},
    {0x01e41e9fc, 0x04c144932}, {0x086d8e4d2, 0x0271d9844},
    {0x160f7af7a, 0x052148f02}, {0x05bb8f1bc, 0x08e766a0c},
    {0x0a90fd27a, 0x0a3c6f37a}, {0x0b3af077a, 0x093a5f730},
    {0x04984d782, 0x1d22c238e}, {0x0ca6ef3ac, 0x06cb08e5c},
    {0x0234e0b26, 0x063ded06a}, {0x1d88abd4a, 0x06b749fb2},
    {0x04597456a, 0x04d56973c}, {0x0e9e28eb4, 0x1167f94f2},
    {0x07b3ff57a, 0x19385bf2e}, {0x0c9c8b782, 0x0cec3662e},
    {0x13a9cba9e, 0x0e417f38a}, {0x093e106a4, 0x19329634a},
    {0x167001a9c, 0x14e727980}, {0x1ddffc5d4, 0x0e6fc4e6a},
    {0x00df04680, 0x0d104b8fc}, {0x02342001e, 0x08227bb8a},
    {0x00a2a8d7e, 0x05b397730}, {0x168763fa6, 0x0b0cd4768},
    {0x1ed5a407a, 0x0e78eb416}, {0x0d2c3ed1a, 0x13c2b89c4},
    {0x0995a5724, 0x1641378f0}, {0x19b1afbc4, 0x0d7a4825c},
    {0x109ffedc0, 0x08d96551c}, {0x0f2271e60, 0x10f5ff2ba},
    {0x00b0bf8ca, 0x00bf80dd2}, {0x123888b7a, 0x00167d312},
    {0x1e888f7dc, 0x18dcddd1c}, {0x002ee03b2, 0x0f6076544},
    {0x183e8d8fe, 0x06a45d2b2}, {0x133d7a042, 0x026f6a60a},
    {0x116b0f50c, 0x1dd3e10e8}, {0x05fabe670, 0x1a2adb74e},
    {0x130004488, 0x0de87806c}, {0x000bcf5f6, 0x19d34af3a},
    {0x18f0c7078, 0x014338754}, {0x017f27698, 0x049c3cc9c},
    {0x058ca5f00, 0x15e3e77ee}, {0x1af900c24, 0x068bce87a},
    {0x0b5cfca28, 0x0dd07448e}, {0x0ded288f8, 0x1524fa6c6},
    {0x059f229bc, 0x1d8048348}, {0x06d390dec, 0x16cba8aca},
    {0x037170390, 0x0a3e3e02c}, {0x06353c1cc, 0x042d98888},
    {0x0c4584f5c, 0x0d73c7bea}, {0x1f16a3418, 0x1329d9f7e},
    {0x0531377e2, 0x185137662}, {0x1d8d9ca7c, 0x1b1c69528},
    {0x0b25b29f2, 0x18a08b5bc}, {0x19fb2a8b0, 0x02178513a},
    {0x1a08fe6ac, 0x1da758ae0}, {0x045cddf4e, 0x0e0ac139e},
    {0x1a91647f2, 0x169cf9eb0}, {0x1a0f717c4, 0x0170076fa},
};

/*
 * CombineCRC performs pclmulqdq multiplication of 2 partial CRC's and a well
 * chosen constant and xor's these with the remaining CRC.
 */
inline __attribute__((__always_inline__))
uint64_t CombineCRC(
    unsigned long block_size,
    uint64_t crc0,
    uint64_t crc1,
    uint64_t crc2,
    const uint64_t* next2) {
  const auto multiplier = *(clmul_constants + block_size - 1);
  const auto crc0_xmm = _mm_set_epi64x(0, crc0);
  const auto res0 = _mm_clmulepi64_si128(crc0_xmm, multiplier, 0x00);
  const auto crc1_xmm = _mm_set_epi64x(0, crc1);
  const auto res1 = _mm_clmulepi64_si128(crc1_xmm, multiplier, 0x10);
  const auto res = _mm_xor_si128(res0, res1);
  crc0 = _mm_cvtsi128_si64(res);
  crc0 = crc0 ^ *((uint64_t*)next2 - 1);
  crc2 = _mm_crc32_u64(crc2, crc0);
  return crc2;
}

// Generates a block that will crc up to 7 bytes of unaligned data.
// Always inline to avoid overhead on small crc sizes.
inline __attribute__((__always_inline__))
void align_to_8(
    unsigned long align,
    uint64_t& crc0, // crc so far, updated on return
    const unsigned char*& next) { // next data pointer, updated on return
  uint32_t crc32bit = crc0;
  if (align & 0x04) {
    crc32bit = _mm_crc32_u32(crc32bit, *(uint32_t*)next);
    next += sizeof(uint32_t);
  }
  if (align & 0x02) {
    crc32bit = _mm_crc32_u16(crc32bit, *(uint16_t*)next);
    next += sizeof(uint16_t);
  }
  if (align & 0x01) {
    crc32bit = _mm_crc32_u8(crc32bit, *(next));
    next++;
  }
  crc0 = crc32bit;
}

// The main loop for large crc sizes. Generates three crc32c
// streams, of varying block sizes, using a duff's device.
void triplet_loop(
    unsigned long block_size,
    uint64_t& crc0, // crc so far, updated on return
    const unsigned char*& next, // next data pointer, updated on return
    unsigned long n) { // block count
  uint64_t crc1 = 0, crc2 = 0;
  // points to the first byte of the next block
  const uint64_t* next0 = (uint64_t*)next + block_size;
  const uint64_t* next1 = next0 + block_size;
  const uint64_t* next2 = next1 + block_size;

  // Use Duff's device, a for() loop inside a switch()
  // statement. This needs to execute at least once, round len
  // down to nearest triplet multiple
  switch (block_size) {
    case 128:
      do {
        // jumps here for a full block of len 128
        CRCtriplet(crc, next, -128);

        // Generates case statements from 127 to 2 of form:
        // case 127:
        //    CRCtriplet(crc, next, -127);
        BOOST_PP_REPEAT_FROM_TO(0, 126, CASEREPEAT_TRIPLET, 126);

        // For the last byte, the three crc32c streams must be combined
        // using carry-less multiplication.
        case 1:
          CRCduplet(crc, next, -1); // the final triplet is actually only 2
          crc0 = CombineCRC(block_size, crc0, crc1, crc2, next2);
          if (--n > 0) {
            crc1 = crc2 = 0;
            block_size = 128;
            // points to the first byte of the next block
            next0 = next2 + 128;
            next1 = next0 + 128; // from here on all blocks are 128 long
            next2 = next1 + 128;
          }
        case 0:;
      } while (n > 0);
  }

  next = (const unsigned char*)next2;
}

} // namespace crc32c_detail

/* Compute CRC-32C using the Intel hardware instruction. */
__attribute__((__target__("sse4.2")))
uint32_t crc32c_hw(const uint8_t* buf, size_t len, uint32_t crc) {
  const unsigned char* next = (const unsigned char*)buf;
  unsigned long count;
  uint64_t crc0;
  crc0 = crc;

  if (len >= 8) {
    // if len > 216 then align and use triplets
    if (len > 216) {
      unsigned long align = (8 - (uintptr_t)next) & 7;
      crc32_detail::align_to_8(align, crc0, next);
      len -= align;

      count = len / 24; // number of triplets
      len %= 24; // bytes remaining
      unsigned long n = count >> 7; // #blocks = first block + full blocks
      unsigned long block_size = count & 127;
      if (block_size == 0) {
        block_size = 128;
      } else {
        n++;
      }

      // This is a separate function call mainly to stop
      // clang from spilling registers.
      crc32_detail::triplet_loop(block_size, crc0, next, n);
    }

    unsigned count2 = len >> 3;
    len = len & 7;
    next += (count2 * 8);

    // Generates a duff device for the last 128 bytes of aligned data.
    switch (count2) {
      // Generates case statements of the form:
      // case 27:
      //   CRCsinglet(crc0, next, -27 * 8);
      BOOST_PP_REPEAT_FROM_TO(0, 27, CASEREPEAT_SINGLET, 27);
      case 0:;
    }
  }

  // compute the crc for up to seven trailing bytes
  crc32_detail::align_to_8(len, crc0, next);
  return (uint32_t)crc0;
}

/*
 * CRC-32 folding with PCLMULQDQ.
 *
 * Copyright 2016 Eric Biggers
 *
 * The basic idea is to repeatedly "fold" each 512 bits into the next
 * 512 bits, producing an abbreviated message which is congruent the
 * original message modulo the generator polynomial G(x).
 *
 * Folding each 512 bits is implemented as eight 64-bit folds, each of
 * which uses one carryless multiplication instruction.  It's expected
 * that CPUs may be able to execute some of these multiplications in
 * parallel.
 *
 * Explanation of "folding": let A(x) be 64 bits from the message, and
 * let B(x) be 95 bits from a constant distance D later in the
 * message.  The relevant portion of the message can be written as:
 *
 *      M(x) = A(x)*x^D + B(x)
 *
 * ... where + and * represent addition and multiplication,
 * respectively, of polynomials over GF(2).  Note that when
 * implemented on a computer, these operations are equivalent to XOR
 * and carryless multiplication, respectively.
 *
 * For the purpose of CRC calculation, only the remainder modulo the
 * generator polynomial G(x) matters:
 *
 * M(x) mod G(x) = (A(x)*x^D + B(x)) mod G(x)
 *
 * Since the modulo operation can be applied anywhere in a sequence of
 * additions and multiplications without affecting the result, this is
 * equivalent to:
 *
 * M(x) mod G(x) = (A(x)*(x^D mod G(x)) + B(x)) mod G(x)
 *
 * For any D, 'x^D mod G(x)' will be a polynomial with maximum degree
 * 31, i.e.  a 32-bit quantity.  So 'A(x) * (x^D mod G(x))' is
 * equivalent to a carryless multiplication of a 64-bit quantity by a
 * 32-bit quantity, producing a 95-bit product.  Then, adding
 * (XOR-ing) the product to B(x) produces a polynomial with the same
 * length as B(x) but with the same remainder as 'A(x)*x^D + B(x)'.
 * This is the basic fold operation with 64 bits.
 *
 * Note that the carryless multiplication instruction PCLMULQDQ
 * actually takes two 64-bit inputs and produces a 127-bit product in
 * the low-order bits of a 128-bit XMM register.  This works fine, but
 * care must be taken to account for "bit endianness".  With the CRC
 * version implemented here, bits are always ordered such that the
 * lowest-order bit represents the coefficient of highest power of x
 * and the highest-order bit represents the coefficient of the lowest
 * power of x.  This is backwards from the more intuitive order.
 * Still, carryless multiplication works essentially the same either
 * way.  It just must be accounted for that when we XOR the 95-bit
 * product in the low-order 95 bits of a 128-bit XMM register into
 * 128-bits of later data held in another XMM register, we'll really
 * be XOR-ing the product into the mathematically higher degree end of
 * those later bits, not the lower degree end as may be expected.
 *
 * So given that caveat and the fact that we process 512 bits per
 * iteration, the 'D' values we need for the two 64-bit halves of each
 * 128 bits of data are:
 *
 * D = (512 + 95) - 64 for the higher-degree half of each 128
 *                 bits, i.e. the lower order bits in
 *                 the XMM register
 *
 *    D = (512 + 95) - 128 for the lower-degree half of each 128
 *                 bits, i.e. the higher order bits in
 *                 the XMM register
 *
 * The required 'x^D mod G(x)' values were precomputed.
 *
 * When <= 512 bits remain in the message, we finish up by folding
 * across smaller distances.  This works similarly; the distance D is
 * just different, so different constant multipliers must be used.
 * Finally, once the remaining message is just 64 bits, it is is
 * reduced to the CRC-32 using Barrett reduction (explained later).
 *
 * For more information see the original paper from Intel: "Fast CRC
 *    Computation for Generic Polynomials Using PCLMULQDQ
 *    Instruction" December 2009
 *    http://www.intel.com/content/dam/www/public/us/en/documents/
 *    white-papers/
 *    fast-crc-computation-generic-polynomials-pclmulqdq-paper.pdf
 */
uint32_t
crc32_hw_aligned(uint32_t remainder, const __m128i* p, size_t vec_count) {
  /* Constants precomputed by gen_crc32_multipliers.c.  Do not edit! */
  const __m128i multipliers_4 = _mm_set_epi32(0, 0x1D9513D7, 0, 0x8F352D95);
  const __m128i multipliers_2 = _mm_set_epi32(0, 0x81256527, 0, 0xF1DA05AA);
  const __m128i multipliers_1 = _mm_set_epi32(0, 0xCCAA009E, 0, 0xAE689191);
  const __m128i final_multiplier = _mm_set_epi32(0, 0, 0, 0xB8BC6765);
  const __m128i mask32 = _mm_set_epi32(0, 0, 0, 0xFFFFFFFF);
  const __m128i barrett_reduction_constants =
      _mm_set_epi32(0x1, 0xDB710641, 0x1, 0xF7011641);

  const __m128i* const end = p + vec_count;
  const __m128i* const end512 = p + (vec_count & ~3);
  __m128i x0, x1, x2, x3;

  /*
   * Account for the current 'remainder', i.e. the CRC of the part of
   * the message already processed.  Explanation: rewrite the message
   * polynomial M(x) in terms of the first part A(x), the second part
   * B(x), and the length of the second part in bits |B(x)| >= 32:
   *
   *    M(x) = A(x)*x^|B(x)| + B(x)
   *
   * Then the CRC of M(x) is:
   *
   *    CRC(M(x)) = CRC(A(x)*x^|B(x)| + B(x))
   *              = CRC(A(x)*x^32*x^(|B(x)| - 32) + B(x))
   *              = CRC(CRC(A(x))*x^(|B(x)| - 32) + B(x))
   *
   * Note: all arithmetic is modulo G(x), the generator polynomial; that's
   * why A(x)*x^32 can be replaced with CRC(A(x)) = A(x)*x^32 mod G(x).
   *
   * So the CRC of the full message is the CRC of the second part of the
   * message where the first 32 bits of the second part of the message
   * have been XOR'ed with the CRC of the first part of the message.
   */
  x0 = *p++;
  x0 = _mm_xor_si128(x0, _mm_set_epi32(0, 0, 0, remainder));

  if (p > end512) /* only 128, 256, or 384 bits of input? */
    goto _128_bits_at_a_time;
  x1 = *p++;
  x2 = *p++;
  x3 = *p++;

  /* Fold 512 bits at a time */
  for (; p != end512; p += 4) {
    __m128i y0, y1, y2, y3;

    y0 = p[0];
    y1 = p[1];
    y2 = p[2];
    y3 = p[3];

    /*
     * Note: the immediate constant for PCLMULQDQ specifies which
     * 64-bit halves of the 128-bit vectors to multiply:
     *
     * 0x00 means low halves (higher degree polynomial terms for us)
     * 0x11 means high halves (lower degree polynomial terms for us)
     */
    y0 = _mm_xor_si128(y0, _mm_clmulepi64_si128(x0, multipliers_4, 0x00));
    y1 = _mm_xor_si128(y1, _mm_clmulepi64_si128(x1, multipliers_4, 0x00));
    y2 = _mm_xor_si128(y2, _mm_clmulepi64_si128(x2, multipliers_4, 0x00));
    y3 = _mm_xor_si128(y3, _mm_clmulepi64_si128(x3, multipliers_4, 0x00));
    y0 = _mm_xor_si128(y0, _mm_clmulepi64_si128(x0, multipliers_4, 0x11));
    y1 = _mm_xor_si128(y1, _mm_clmulepi64_si128(x1, multipliers_4, 0x11));
    y2 = _mm_xor_si128(y2, _mm_clmulepi64_si128(x2, multipliers_4, 0x11));
    y3 = _mm_xor_si128(y3, _mm_clmulepi64_si128(x3, multipliers_4, 0x11));

    x0 = y0;
    x1 = y1;
    x2 = y2;
    x3 = y3;
  }

  /* Fold 512 bits => 128 bits */
  x2 = _mm_xor_si128(x2, _mm_clmulepi64_si128(x0, multipliers_2, 0x00));
  x3 = _mm_xor_si128(x3, _mm_clmulepi64_si128(x1, multipliers_2, 0x00));
  x2 = _mm_xor_si128(x2, _mm_clmulepi64_si128(x0, multipliers_2, 0x11));
  x3 = _mm_xor_si128(x3, _mm_clmulepi64_si128(x1, multipliers_2, 0x11));
  x3 = _mm_xor_si128(x3, _mm_clmulepi64_si128(x2, multipliers_1, 0x00));
  x3 = _mm_xor_si128(x3, _mm_clmulepi64_si128(x2, multipliers_1, 0x11));
  x0 = x3;

_128_bits_at_a_time:
  while (p != end) {
    /* Fold 128 bits into next 128 bits */
    x1 = *p++;
    x1 = _mm_xor_si128(x1, _mm_clmulepi64_si128(x0, multipliers_1, 0x00));
    x1 = _mm_xor_si128(x1, _mm_clmulepi64_si128(x0, multipliers_1, 0x11));
    x0 = x1;
  }

  /* Now there are just 128 bits left, stored in 'x0'. */

  /*
   * Fold 128 => 96 bits.  This also implicitly appends 32 zero bits,
   * which is equivalent to multiplying by x^32.  This is needed because
   * the CRC is defined as M(x)*x^32 mod G(x), not just M(x) mod G(x).
   */
  x0 = _mm_xor_si128(_mm_srli_si128(x0, 8), _mm_clmulepi64_si128(x0, multipliers_1, 0x10));

  /* Fold 96 => 64 bits */
  x0 = _mm_xor_si128(_mm_srli_si128(x0, 4),
      _mm_clmulepi64_si128(_mm_and_si128(x0, mask32), final_multiplier, 0x00));

  /*
   * Finally, reduce 64 => 32 bits using Barrett reduction.
   *
   * Let M(x) = A(x)*x^32 + B(x) be the remaining message.  The goal is to
   * compute R(x) = M(x) mod G(x).  Since degree(B(x)) < degree(G(x)):
   *
   *    R(x) = (A(x)*x^32 + B(x)) mod G(x)
   *         = (A(x)*x^32) mod G(x) + B(x)
   *
   * Then, by the Division Algorithm there exists a unique q(x) such that:
   *
   *    A(x)*x^32 mod G(x) = A(x)*x^32 - q(x)*G(x)
   *
   * Since the left-hand side is of maximum degree 31, the right-hand side
   * must be too.  This implies that we can apply 'mod x^32' to the
   * right-hand side without changing its value:
   *
   *    (A(x)*x^32 - q(x)*G(x)) mod x^32 = q(x)*G(x) mod x^32
   *
   * Note that '+' is equivalent to '-' in polynomials over GF(2).
   *
   * We also know that:
   *
   *                  / A(x)*x^32 \
   *    q(x) = floor (  ---------  )
   *                  \    G(x)   /
   *
   * To compute this efficiently, we can multiply the top and bottom by
   * x^32 and move the division by G(x) to the top:
   *
   *                  / A(x) * floor(x^64 / G(x)) \
   *    q(x) = floor (  -------------------------  )
   *                  \           x^32            /
   *
   * Note that floor(x^64 / G(x)) is a constant.
   *
   * So finally we have:
   *
   *                              / A(x) * floor(x^64 / G(x)) \
   *    R(x) = B(x) + G(x)*floor (  -------------------------  )
   *                              \           x^32            /
   */
  x1 = x0;
  x0 = _mm_clmulepi64_si128(_mm_and_si128(x0, mask32), barrett_reduction_constants, 0x00);
  x0 = _mm_clmulepi64_si128(_mm_and_si128(x0, mask32), barrett_reduction_constants, 0x10);
  return _mm_cvtsi128_si32(_mm_srli_si128(_mm_xor_si128(x0, x1), 4));
}

// Fast SIMD implementation of CRC-32 for x86 with pclmul
uint32_t
crc32_hw(const uint8_t* data, size_t nbytes, uint32_t startingChecksum) {
  uint32_t sum = startingChecksum;
  size_t offset = 0;

  // Process unaligned bytes
  if ((uintptr_t)data & 15) {
    size_t limit = std::min(nbytes, -(uintptr_t)data & 15);
    sum = crc32_sw(data, limit, sum);
    offset += limit;
    nbytes -= limit;
  }

  if (nbytes >= 16) {
    sum = crc32_hw_aligned(sum, (const __m128i*)(data + offset), nbytes / 16);
    offset += nbytes & ~15;
    nbytes &= 15;
  }

  // Remaining unaligned bytes
  return crc32_sw(data + offset, nbytes, sum);
}

} // namespace detail

uint32_t crc32c(const uint8_t *data, size_t nbytes,
    uint32_t startingChecksum) {
  return detail::crc32c_hw(data, nbytes, startingChecksum);
}

uint32_t crc32(const uint8_t* data, size_t nbytes, uint32_t startingChecksum) {
  return detail::crc32_hw(data, nbytes, startingChecksum);
}

uint32_t
crc32_type(const uint8_t* data, size_t nbytes, uint32_t startingChecksum) {
  return ~crc32(data, nbytes, startingChecksum);
}

} // namespace rdd
