// Copyright 2020 multiparty.org

#include "drivacy/primitives/secret_sharing.h"

#include <cstdlib>
#include <iostream>

int main() {
  for (int i = 0; i < 100; i++) {
    uint64_t query = std::rand() % drivacy::primitives::PRIME;
    uint64_t parties = std::rand() % 5 + 2;  // [2, 7)

    auto shares = drivacy::primitives::IncrementalSecretShare(query, parties);
    uint64_t reconstruct =
        drivacy::primitives::IncrementalReconstruct(shares, parties);
    if (query != reconstruct) {
      std::cout << "Test failed!" << std::endl;
      std::cout << "Query: " << query << " Parties: " << parties << std::endl;
      std::cout << "Reconstructed: " << reconstruct << std::endl;

      return 1;
    }
  }

  std::cout << "All test pass!" << std::endl;

  return 0;
}
