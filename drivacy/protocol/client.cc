// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#include "drivacy/protocol/client.h"

#include <iostream>
#include <unordered_map>
#include <vector>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/crypto.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace client {

// Used to generate unique tags to link queries to responses.
uint64_t tag = 0;

// Maps query tag to the preshare meant for them.
std::unordered_map<uint64_t, uint64_t> tag_to_preshare;

types::Query CreateQuery(uint64_t value, const types::Configuration &config) {
  uint64_t query_tag = tag++;
  uint64_t parties = config.parties();

  types::Query query;
  query.set_tag(query_tag);
  query.set_tally(1);

  // Share query value and create additive preshares of the response.
  auto shares = primitives::GenerateIncrementalSecretShares(value, parties);
  auto preshares = primitives::GenerateAdditiveSecretShares(0, parties + 1);
  tag_to_preshare.insert({query_tag, preshares.at(parties)});

  // Combine shares into a single struct.
  std::vector<types::QueryShare> query_shares;
  query_shares.reserve(shares.size());
  for (size_t i = 0; i < shares.size(); i++) {
    query_shares.push_back({shares.at(i).x, shares.at(i).y, preshares.at(i)});
  }

  // Onion encrypt shares and place them in query.
  primitives::crypto::OnionEncrypt(query_shares, config, &query);

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
