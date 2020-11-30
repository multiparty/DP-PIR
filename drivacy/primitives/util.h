// Copyright 2020 multiparty.org

// Utils for secret sharing.

#ifndef DRIVACY_PRIMITIVES_UTIL_H_
#define DRIVACY_PRIMITIVES_UTIL_H_

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <random>

namespace drivacy {
namespace primitives {
namespace util {

inline uint32_t Prime() {
  // 26 bits prime. // 29 bits prime. // 26 bits prime.
  return 67108859;  // 536870909;     // 67108859;
}

inline float RandUniform() {
  return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

// Random number using std::rand() (unseeded)
// TODO(babman): replace this with something proper.
inline uint32_t Rand32(uint32_t lower_bound, uint32_t upper_bound) {
  return (std::rand() % (upper_bound - lower_bound)) + lower_bound;
}

// Random number with a seed.
using Generator = std::mt19937;

inline Generator SeedGenerator(uint32_t seed) { return std::mt19937(seed); }
inline uint32_t SRand32(Generator *generator, uint32_t lower_bound,
                       uint32_t upper_bound) {
  uint64_t span = upper_bound - lower_bound;
  uint64_t generator_span = std::mt19937::max() - std::mt19937::min();
  generator_span += 1;  // std::mt19937::max is inclusive.
  // assert domain is large enough for rejection sampling!
  assert(span <= generator_span);
  // rejection sampling
  uint64_t rejection = generator_span - (generator_span % span);
  assert((rejection % span) == 0);
  uint64_t result;
  while ((result = (*generator)()) >= rejection + std::mt19937::min()) {
  }
  result -= std::mt19937::min();  // result uniformly  in range [0, rejection)
                                  // with rejection % span == 0
  result = result % span;         // result in range [0, span) uniformly
  result += lower_bound;          // result in range [lower_bound, upper_bound)
  return result;
}

}  // namespace util
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_UTIL_H_
