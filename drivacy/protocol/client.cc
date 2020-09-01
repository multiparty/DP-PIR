// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#include "drivacy/protocol/client.h"

#include <unordered_map>
#include <vector>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/crypto.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace client {

// Maps query tag to the preshare meant for them.
std::unordered_map<uint64_t, uint64_t> tag_to_preshare;

types::Query CreateQuery(uint64_t value, const types::Configuration &config,
                         types::ClientState *state) {
  uint64_t query_tag = state->tag++;
  uint64_t parties = config.parties();

  types::Query query;
  query.set_tag(query_tag);
  query.set_tally(1);

  // Share query value and create additive preshares of the response.
  auto shares = primitives::GenerateIncrementalSecretShares(value, parties);
  auto preshares = primitives::GenerateAdditiveSecretShares(0, parties + 1);

  // Combine shares into a single struct.
  std::vector<types::QueryShare> query_shares;
  query_shares.reserve(shares.size());
  for (size_t i = 0; i < shares.size(); i++) {
    query_shares.push_back({shares.at(i).x, shares.at(i).y, preshares.at(i)});
  }

  // Store state.
  state->tag_to_query_value[query_tag] = value;
  state->tag_to_query_preshare[query_tag] = preshares.at(parties);

  // Onion encrypt shares and place them in query.
  primitives::crypto::OnionEncrypt(query_shares, config, &query);

  return query;
}

std::pair<uint64_t, uint64_t> ReconstructResponse(
    const types::Response &response, types::ClientState *state) {
  uint64_t preshare = state->tag_to_query_preshare.at(response.tag());
  uint64_t query = state->tag_to_query_value.at(response.tag());

  uint64_t output = primitives::AdditiveReconstruct(response.tally(), preshare);
  return std::make_pair(query, output);
}

}  // namespace client
}  // namespace protocol
}  // namespace drivacy