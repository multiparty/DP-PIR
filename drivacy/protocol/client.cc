// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#include "drivacy/protocol/client.h"

#include <iostream>
#include <unordered_map>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace client {

// Used to generate unique tags to link queries to responses.
uint64_t tag = 0;

// Maps query tag to the preshare meant for them.
std::unordered_map<uint64_t, uint64_t> tag_to_preshare;

types::Query CreateQuery(uint64_t value, uint32_t parties) {
  uint64_t query_tag = tag++;

  types::Query query;
  query.set_tag(query_tag);
  query.set_tally(1);
  // Share query value.
  auto shares = primitives::GenerateIncrementalSecretShares(value, parties);
  for (const auto &share : shares) {
    types::QueryShare *query_share = query.add_shares();
    query_share->set_x(share.x);
    query_share->set_y(share.y);
  }
  // Create additive pre-shares for response, save one.
  auto preshares = primitives::GenerateAdditiveSecretShares(0, parties + 1);
  tag_to_preshare.insert({query_tag, preshares.at(parties)});
  preshares.pop_back();
  for (const auto &preshare : preshares) {
    query.add_preshares(preshare);
  }
  return query;
}

void ReconstructResponse(const types::Response &response) {
  uint64_t preshare = tag_to_preshare.at(response.tag());
  uint64_t output = primitives::AdditiveReconstruct(response.tally(), preshare);
  std::cout << "\tclient response: " << output << std::endl;
}

}  // namespace client
}  // namespace protocol
}  // namespace drivacy
