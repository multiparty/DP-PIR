// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#include "drivacy/protocol/offline/client.h"

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace offline {
namespace client {

std::vector<types::Message> SampleCommonReference(uint32_t seed,
                                                  uint32_t party_id,
                                                  uint32_t party_count) {
  std::vector<types::Message> result;

  // Create all preshares.
  uint32_t numparties = party_count - party_id;
  std::vector<uint32_t> additive_shares =
      primitives::GenerateAdditiveSecretShares(numparties);
  std::vector<types::IncrementalSecretShare> incremental_shares =
      primitives::PreIncrementalSecretShares(numparties);

  // Fill shares and tags in.
  types::Tag tag = seed;  // TODO(babman): fix this to be unique.
  for (uint32_t i = 0; i < party_count - party_id; i++) {
    types::Tag next_tag = tag;
    result.emplace_back(tag, next_tag, incremental_shares.at(i),
                        additive_shares.at(i));
    tag = next_tag;
  }

  return result;
}

}  // namespace client
}  // namespace offline
}  // namespace protocol
}  // namespace drivacy
