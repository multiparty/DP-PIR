// Copyright 2020 multiparty.org

// Utils for secret sharing.

#ifndef DRIVACY_PRIMITIVES_UTIL_H_
#define DRIVACY_PRIMITIVES_UTIL_H_

#include <cstdint>

namespace drivacy {
namespace primitives {
namespace util {

inline uint64_t Prime() {
  // 26 bits prime.
  return 67108859;
}

inline uint64_t Mod(uint64_t a, uint64_t modulus) {
  return a % modulus > 0 ? a : a + modulus;
}

}  // namespace util
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_UTIL_H_
