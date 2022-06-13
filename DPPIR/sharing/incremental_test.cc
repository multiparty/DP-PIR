// Tests our incremental secret sharing implementation for correctness.
//
// Repeatedly comes up with random values in [0, prime), and secret shares
// them using our scheme, it then incrementally reconstructs the value
// from the shares, and tests that the final reconstruction is equal to the
// initial value.

#include "DPPIR/sharing/incremental.h"

#include <iostream>

// NOLINTNEXTLINE
#include "sodium.h"

namespace DPPIR {
namespace sharing {

key_t Test(key_t value, size_t numparties) {
  // Pre-secret share into numparties-many shares.
  auto shares = PreIncrementalSecretShares(numparties);

  // Compute the final piece of the sharing using value and preshares.
  incremental_tally_t tally = GenerateIncrementalTally(value, shares);

  // Reconstruct the value incrementally, in order of shares.
  for (const auto &share : shares) {
    // Tally is updated by every incremental reconstruction.
    tally = IncrementalReconstruct(tally, share);
  }

  // Tally should be equal to value after all reconstructions are executed.
  return tally;
}

}  // namespace sharing
}  // namespace DPPIR

int main() {
  for (int i = 0; i < 100000; i++) {
    size_t numparties = randombytes_uniform(5) + 2;
    DPPIR::key_t value = randombytes_uniform(INCREMENTAL_PRIME);
    DPPIR::key_t reconstructed = DPPIR::sharing::Test(value, numparties);
    if (value != reconstructed) {
      std::cout << "Test failed!" << std::endl;
      std::cout << "Value: " << value << " Parties: " << numparties
                << std::endl;
      std::cout << "Reconstructed: " << reconstructed << std::endl;

      return 1;
    }
  }

  // Special cases: 0 and prime - 1.
  if (DPPIR::sharing::Test(0, 5) != 0) {
    std::cout << "Test failed!" << std::endl;
    std::cout << "0" << std::endl;
    return 1;
  }
  if (DPPIR::sharing::Test(INCREMENTAL_PRIME - 1, 5) != INCREMENTAL_PRIME - 1) {
    std::cout << "Test failed!" << std::endl;
    std::cout << "INCREMENTAL_PRIME - 1" << std::endl;
    return 1;
  }

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
