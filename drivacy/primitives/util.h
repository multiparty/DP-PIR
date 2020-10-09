// Copyright 2020 multiparty.org

// Utils for secret sharing.

#ifndef DRIVACY_PRIMITIVES_UTIL_H_
#define DRIVACY_PRIMITIVES_UTIL_H_

#include <cstdint>
#include <cstdlib>

namespace drivacy {
namespace primitives {
namespace util {

inline uint64_t Prime() {
  // 29 bits prime. // 26 bits prime.
  return 536870909;  // 67108859;
}

inline uint64_t Mod(uint64_t a, uint64_t modulus) {
  return a < 0 ? (a % modulus + modulus) : a + modulus;
}

inline uint64_t Rand64(uint64_t lower_bound, uint64_t upper_bound) {
  return (std::rand() % (upper_bound - lower_bound)) + lower_bound;
}

inline uint32_t Rand32(uint32_t lower_bound, uint32_t upper_bound) {
  return (std::rand() % (upper_bound - lower_bound)) + lower_bound;
}

}  // namespace util
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_UTIL_H_
