#include "drivacy/primitives/secret_sharing.h"

#include <cstdlib>

namespace drivacy {
namespace primitives {

uint64_t PRIME = 0xffffffffffffffff - 58;  // 2^64 - 59 is prime.

namespace {

inline uint64_t mod(uint64_t a, uint64_t modulus) {
  return a % modulus > 0 ? a : a + modulus;
}

}  // namespace

uint64_t** generateShares(uint64_t query, uint64_t numparty) {
  uint64_t** shares = (uint64_t **) malloc(2* sizeof(uint64_t *));
  shares[0] = (uint64_t *) malloc(numparty * sizeof(uint64_t));
  shares[1] = (uint64_t *) malloc(numparty * sizeof(uint64_t));

  uint64_t t = 1;
  for (size_t i = 0; i < numparty-1; i++){
      shares[0][i] = std::rand() % PRIME + 1;
      shares[1][i] = std::rand() % PRIME + 1;
      t = (t * shares[1][i] + shares[0][i]) % PRIME;
  }
  shares[1][numparty-1] = std::rand() % PRIME + 1;
  shares[0][numparty-1] = mod(query - t*shares[1][numparty-1], PRIME);

  return shares;
}

uint64_t reconstructSecret(uint64_t** shares, uint64_t numparty) {
  uint64_t query = 1;
  for(size_t i = 0; i < numparty; i++){
    query = (shares[1][i]*query + shares[0][i]) % PRIME;
  }

  return query;
}

}  // namespace primitives
}  // namespace drivacy


