/*
 * Copyright (C) 1996, 1997, 1998 Theodore Ts'o.
 * Copyright (C) 2017, Yeolar
 */

#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <net/if.h>
#include <netinet/in.h>

#include "raster/util/String.h"
#include "raster/util/Uuid.h"

namespace rdd {

static __thread unsigned short ul_jrand_seed[3];

int randomGetFd() {
  int i, fd;
  struct timeval tv;

  fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
  if (fd == -1)
    fd = open("/dev/random", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (fd >= 0) {
    i = fcntl(fd, F_GETFD);
    if (i >= 0)
      fcntl(fd, F_SETFD, i | FD_CLOEXEC);
  }

  gettimeofday(&tv, 0);
  srand((getpid() << 16) ^ getuid() ^ tv.tv_sec ^ tv.tv_usec);

  ul_jrand_seed[0] = getpid() ^ (tv.tv_sec & 0xFFFF);
  ul_jrand_seed[1] = getppid() ^ (tv.tv_usec & 0xFFFF);
  ul_jrand_seed[2] = (tv.tv_sec ^ tv.tv_usec) >> 16;
  /* Crank the random number generator a few times */
  gettimeofday(&tv, 0);
  for (i = (tv.tv_sec ^ tv.tv_usec) & 0x1F; i > 0; i--)
    rand();
  return fd;
}

/*
 * Generate a stream of random nbytes into buf.
 * Use /dev/urandom if possible, and if not,
 * use glibc pseudo-random functions.
 */
void randomGetBytes(void *buf, size_t nbytes) {
  size_t i, n = nbytes;
  int fd = randomGetFd();
  int lose_counter = 0;
  unsigned char *cp = (unsigned char *)buf;

  if (fd >= 0) {
    while (n > 0) {
      ssize_t x = read(fd, cp, n);
      if (x <= 0) {
        if (lose_counter++ > 16)
          break;
        continue;
      }
      n -= x;
      cp += x;
      lose_counter = 0;
    }
    close(fd);
  }

  /*
   * We do this all the time, but this is the only source of
   * randomness if /dev/random/urandom is out to lunch.
   */
  for (cp = (unsigned char*)buf, i = 0; i < nbytes; i++)
    *cp++ ^= (rand() >> 7) & 0xFF;

  unsigned short tmp_seed[3];

  memcpy(tmp_seed, ul_jrand_seed, sizeof(tmp_seed));
  ul_jrand_seed[2] = ul_jrand_seed[2] ^ syscall(__NR_gettid);
  for (cp = (unsigned char*)buf, i = 0; i < nbytes; i++)
    *cp++ ^= (jrand48(tmp_seed) >> 7) & 0xFF;
  memcpy(ul_jrand_seed, tmp_seed,
         sizeof(ul_jrand_seed) - sizeof(unsigned short));
}

void uuidPack(const struct uuid *uu, uuid_t ptr) {
  uint32_t tmp;
  unsigned char *out = ptr;

  tmp = uu->time_low;
  out[3] = (unsigned char)tmp; tmp >>= 8;
  out[2] = (unsigned char)tmp; tmp >>= 8;
  out[1] = (unsigned char)tmp; tmp >>= 8;
  out[0] = (unsigned char)tmp;

  tmp = uu->time_mid;
  out[5] = (unsigned char)tmp; tmp >>= 8;
  out[4] = (unsigned char)tmp;

  tmp = uu->time_hi_and_version;
  out[7] = (unsigned char)tmp; tmp >>= 8;
  out[6] = (unsigned char)tmp;

  tmp = uu->clock_seq;
  out[9] = (unsigned char)tmp; tmp >>= 8;
  out[8] = (unsigned char)tmp;

  memcpy(out + 10, uu->node, 6);
}

void uuidUnpack(const uuid_t in, struct uuid *uu) {
  const uint8_t *ptr = in;
  uint32_t tmp;

  tmp = *ptr++;
  tmp = (tmp << 8) | *ptr++;
  tmp = (tmp << 8) | *ptr++;
  tmp = (tmp << 8) | *ptr++;
  uu->time_low = tmp;

  tmp = *ptr++;
  tmp = (tmp << 8) | *ptr++;
  uu->time_mid = tmp;

  tmp = *ptr++;
  tmp = (tmp << 8) | *ptr++;
  uu->time_hi_and_version = tmp;

  tmp = *ptr++;
  tmp = (tmp << 8) | *ptr++;
  uu->clock_seq = tmp;

  memcpy(uu->node, ptr, 6);
}

/*
 * Get the ethernet hardware address, if we can find it...
 */
static int getNodeId(unsigned char *node_id) {
  int sd;
  struct ifreq ifr, *ifrp;
  struct ifconf ifc;
  char buf[1024];
  int n, i;
  unsigned char *a;

  sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sd < 0) {
    return -1;
  }
  memset(buf, 0, sizeof(buf));
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(sd, SIOCGIFCONF, (char *)&ifc) < 0) {
    close(sd);
    return -1;
  }
  n = ifc.ifc_len;
  for (i = 0; i < n; i += sizeof(*ifrp)) {
    ifrp = (struct ifreq *)((char *)ifc.ifc_buf + i);
    strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0)
      continue;
    a = (unsigned char *)&ifr.ifr_hwaddr.sa_data;
    if (!a[0] && !a[1] && !a[2] && !a[3] && !a[4] && !a[5])
      continue;
    if (node_id) {
      memcpy(node_id, a, 6);
      close(sd);
      return 1;
    }
  }
  close(sd);
  return 0;
}

/* Assume that the gettimeofday() has microsecond granularity */
#define MAX_ADJUSTMENT 10

/*
 * Get clock from global sequence clock counter.
 *
 * Return -1 if the clock counter could not be opened/locked (in this case
 * pseudorandom value is returned in @ret_clock_seq), otherwise return 0.
 */
static int getClock(uint32_t *clock_high, uint32_t *clock_low,
                    uint16_t *ret_clock_seq, int *num) {
  static __thread int adjustment = 0;
  static __thread struct timeval last = {0, 0};
  static __thread uint16_t clock_seq;
  struct timeval tv;
  uint64_t clock_reg;

  if ((last.tv_sec == 0) && (last.tv_usec == 0)) {
    randomGetBytes(&clock_seq, sizeof(clock_seq));
    clock_seq &= 0x3FFF;
    gettimeofday(&last, 0);
    last.tv_sec--;
  }

try_again:
  gettimeofday(&tv, 0);
  if ((tv.tv_sec < last.tv_sec) ||
      ((tv.tv_sec == last.tv_sec) && (tv.tv_usec < last.tv_usec))) {
    clock_seq = (clock_seq + 1) & 0x3FFF;
    adjustment = 0;
    last = tv;
  } else if ((tv.tv_sec == last.tv_sec) && (tv.tv_usec == last.tv_usec)) {
    if (adjustment >= MAX_ADJUSTMENT)
      goto try_again;
    adjustment++;
  } else {
    adjustment = 0;
    last = tv;
  }

  clock_reg = tv.tv_usec * 10 + adjustment;
  clock_reg += ((uint64_t)tv.tv_sec) * 10000000;
  clock_reg += (((uint64_t)0x01B21DD2) << 32) + 0x13814000;

  if (num && (*num > 1)) {
    adjustment += *num - 1;
    last.tv_usec += adjustment / 10;
    adjustment = adjustment % 10;
    last.tv_sec += last.tv_usec / 1000000;
    last.tv_usec = last.tv_usec % 1000000;
  }

  *clock_high = clock_reg >> 32;
  *clock_low = clock_reg;
  *ret_clock_seq = clock_seq;
  return -1;  /* compatible with libuuid */
}

int internalUuidGenerateTime(uuid_t out, int *num) {
  static unsigned char node_id[6];
  static int has_init = 0;
  struct uuid uu;
  uint32_t clock_mid;
  int ret;

  if (!has_init) {
    if (getNodeId(node_id) <= 0) {
      randomGetBytes(node_id, 6);
      /*
       * Set multicast bit, to prevent conflicts
       * with IEEE 802 addresses obtained from
       * network cards
       */
      node_id[0] |= 0x01;
    }
    has_init = 1;
  }
  ret = getClock(&clock_mid, &uu.time_low, &uu.clock_seq, num);
  uu.clock_seq |= 0x8000;
  uu.time_mid = (uint16_t)clock_mid;
  uu.time_hi_and_version = ((clock_mid >> 16) & 0x0FFF) | 0x1000;
  memcpy(uu.node, node_id, 6);
  uuidPack(&uu, out);
  return ret;
}

/*
 * Generate time-based UUID and store it to @out
 */
int uuidGenerateTime(uuid_t out) {
  static __thread int num = 0;
  static __thread struct uuid uu;
  static __thread time_t last_time = 0;
  time_t now;

  if (num > 0) {
    now = time(0);
    if (now > last_time + 1)
      num = 0;
  }
  if (num <= 0) {
    num = 0;
  }
  if (num > 0) {
    uu.time_low++;
    if (uu.time_low == 0) {
      uu.time_mid++;
      if (uu.time_mid == 0)
        uu.time_hi_and_version++;
    }
    num--;
    uuidPack(&uu, out);
    return 0;
  }

  return internalUuidGenerateTime(out, 0);
}

std::string uuidGenerateTime() {
  std::string out;

  uuid_t uu;
  uuidGenerateTime(uu);

  StringPiece sp((char*)uu, sizeof(uuid_t));
  hexlify(sp.subpiece(0, 4), out, true); sp.advance(4), out += '-';
  hexlify(sp.subpiece(0, 2), out, true); sp.advance(2), out += '-';
  hexlify(sp.subpiece(0, 2), out, true); sp.advance(2), out += '-';
  hexlify(sp.subpiece(0, 2), out, true); sp.advance(2), out += '-';
  hexlify(sp.subpiece(0, 6), out, true);

  return out;
}

} // namespace rdd
