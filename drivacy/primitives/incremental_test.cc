// Copyright 2020 multiparty.org

// Tests our incremental secret sharing implementation for correctness.
//
// Repeatedly comes up with random values in [0, prime), and secret shares
// them using our scheme, it then incrementally reconstructs the value
// from the shares, and tests that the final reconstruction is equal to the
// initial value.

#include "drivacy/primitives/incremental.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "drivacy/primitives/util.h"
#include "drivacy/types/types.h"

uint32_t Test(uint64_t value, uint64_t numparties) {
  // Pre-secret share into numparties-many shares.
  auto shares = drivacy::primitives::PreIncrementalSecretShares(numparties);
  std::vector<drivacy::types::Message> wrappers;
  for (auto &share : shares) {
    wrappers.emplace_back(0, 0, share, 0);
  }

  // Compute the final piece of the sharing using value and preshares.
  uint32_t tally =
      drivacy::primitives::GenerateIncrementalSecretShares(value, wrappers);

  // Reconstruct the value incrementally, in order of shares.
  for (const auto &share : shares) {
    // Tally is updated by every incremental reconstruction.
    tally = drivacy::primitives::IncrementalReconstruct(tally, share);
  }

  // Tally should be equal to value after all reconstructions are executed.
  return tally;
}

int main() {
  for (int i = 0; i < 100; i++) {
    uint32_t numparties = drivacy::primitives::util::Rand32(2, 7);
    uint32_t value = drivacy::primitives::util::Rand32(
        0, drivacy::primitives::util::Prime());
    uint32_t reconstructed = Test(value, numparties);
    if (value != reconstructed) {
      std::cout << "Test failed!" << std::endl;
      std::cout << "Value: " << value << " Parties: " << numparties
                << std::endl;
      std::cout << "Reconstructed: " << reconstructed << std::endl;

      return 1;
    }
  }

  std::cout << "All tests passed!" << std::endl;

  return 0;
}
