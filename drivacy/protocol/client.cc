// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#include "drivacy/protocol/client.h"

#include <vector>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/crypto.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace client {

types::OutgoingQuery CreateQuery(uint64_t value,
                                 const types::Configuration &config) {
  uint64_t parties = config.parties();

  // Share query value and create additive preshares of the response.
  auto shares = primitives::GenerateIncrementalSecretShares(value, parties);
  auto preshares = primitives::GenerateAdditiveSecretShares(0, parties + 1);

  // Combine shares into a single struct.
  std::vector<types::QueryShare> query_shares;
  query_shares.reserve(shares.size());
  for (size_t i = 0; i < shares.size(); i++) {
    query_shares.push_back({shares.at(i).x, shares.at(i).y, preshares.at(i)});
  }

  // Create the outgoing query object.
  types::OutgoingQuery query(0, parties);
  query.set_tally(1);
  query.set_preshare(preshares.at(parties));
  primitives::crypto::OnionEncrypt(query_shares, config,
                                   query.buffer() + sizeof(types::QueryShare));
  return query;
}

uint64_t ReconstructResponse(const types::Response &response,
                             uint64_t preshare) {
  uint64_t tally = response.tally();
  return primitives::AdditiveReconstruct(tally, preshare);
}

}  // namespace client
}  // namespace protocol
}  // namespace drivacy
