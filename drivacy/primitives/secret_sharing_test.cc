// Copyright 2020 multiparty.org

// Tests our incremental secret sharing implementation for correctness.
//
// Repeatedly comes up with random values in [0, prime), and secret shares
// them using our scheme, it then incrementally reconstructs the value
// from the shares, and tests that the final reconstruction is equal to the
// initial value.

#include "drivacy/primitives/secret_sharing.h"

#include <cstdlib>
#include <iostream>

uint64_t Test(uint64_t value, uint64_t numparties) {
  // Secret share value into numparties-many shares.
  auto shares =
      drivacy::primitives::GenerateIncrementalSecretShares(value, numparties);

  // Reconstruct the value incrementally, in order of shares.
  uint64_t tally = 1;  // Initially, tally = 1.
  for (const auto &share : shares) {
    // Tally is updated by every incremental reconstruction.
    tally = drivacy::primitives::IncrementalReconstruct(tally, share);
  }

  // Tally should be equal to value after all reconstructions are executed.
  return tally;
}

int main() {
  for (int i = 0; i < 100; i++) {
    uint64_t value = std::rand() % drivacy::primitives::PRIME;
    uint32_t numparties = std::rand() % 5 + 2;  // [2, 7)
    uint64_t reconstructed = Test(value, numparties);
    if (value != reconstructed) {
      std::cout << "Test failed!" << std::endl;
      std::cout << "Value: " << value << " Parties: " << numparties
                << std::endl;
      std::cout << "Reconstructed: " << reconstructed << std::endl;

      return 1;
    }
  }

  std::cout << "All test pass!" << std::endl;

  return 0;
}
