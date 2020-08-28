// Copyright 2020 multiparty.org

// Utils for secret sharing.

#ifndef DRIVACY_PRIMITIVES_UTIL_H_
#define DRIVACY_PRIMITIVES_UTIL_H_

#include <cstdint>

namespace drivacy {
namespace primitives {
namespace util {

inline uint64_t Prime() {
  // 2^64 - 59 is the largest prime that fits in 64 bits.
  return 0xffffffffffffffff - 58;
}

inline uint64_t Mod(uint64_t a, uint64_t modulus) {
  return a % modulus > 0 ? a : a + modulus;
}

}  // namespace util
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_UTIL_H_
