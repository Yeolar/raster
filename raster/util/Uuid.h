/*
 * Copyright (C) 1996, 1997, 1998 Theodore Ts'o.
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <string>

/* UUID Variant definitions */
#define UUID_VARIANT_NCS 0
#define UUID_VARIANT_DCE 1
#define UUID_VARIANT_MICROSOFT 2
#define UUID_VARIANT_OTHER 3

/* UUID Type definitions */
#define UUID_TYPE_DCE_TIME 1
#define UUID_TYPE_DCE_RANDOM 4

namespace rdd {

typedef unsigned char uuid_t[16];

struct uuid {
  uint32_t time_low;
  uint16_t time_mid;
  uint16_t time_hi_and_version;
  uint16_t clock_seq;
  uint8_t node[6];
};

void uuidPack(const struct uuid *uu, uuid_t ptr);
void uuidUnpack(const uuid_t in, struct uuid *uu);

inline void uuidClear(uuid_t uu) {
  memset(uu, 0, 16);
}

int uuidGenerateTime(uuid_t out);

std::string uuidGenerateTime();

inline std::string generateUuid(const std::string& upstreamUuid,
                                const std::string& prefix) {
  return upstreamUuid.empty() ? prefix + ':' + uuidGenerateTime()
                              : upstreamUuid;
}

} // namespace rdd
