// Copyright 2020 multiparty.org

// Tests our incremental secret sharing implementation for correctness.
//
// Repeatedly comes up with random values in [0, prime), and secret shares
// them using our scheme, it then incrementally reconstructs the value
// from the shares, and tests that the final reconstruction is equal to the
// initial value.

#include "DPPIR/sharing/additive.h"

#include <iostream>

// NOLINTNEXTLINE
#include "sodium.h"

namespace DPPIR {
namespace sharing {

Response Test(Response value, size_t numparties) {
  // Additively secret share value into numparties-many shares.
  auto shares = GenerateAdditiveSecretShares(numparties);

  // Reconstruct the value incrementally, in order of shares.
  for (const auto& share : shares) {
    // Tally is updated by every incremental reconstruction.
    AdditiveReconstruct(value, share, &value);
  }

  // Tally should be equal to value after all reconstructions are executed.
  return value;
}

}  // namespace sharing
}  // namespace DPPIR

int main() {
  for (int i = 0; i < 100; i++) {
    DPPIR::Response value;
    randombytes_buf(reinterpret_cast<char*>(&value), sizeof(value));
    size_t numparties = randombytes_uniform(5) + 2;
    DPPIR::Response reconstructed = DPPIR::sharing::Test(value, numparties);
    if (value != reconstructed) {
      std::cout << "Test failed!" << std::endl;
      std::cout << "Parties: " << numparties << std::endl;
      std::cout << "Value: " << value << std::endl;
      std::cout << "Reconstructed: " << reconstructed << std::endl;
      return 1;
    }
  }

  std::cout << "All tests passed!" << std::endl;

  return 0;
}
