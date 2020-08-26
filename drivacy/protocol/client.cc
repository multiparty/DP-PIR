// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#include "drivacy/protocol/client.h"

#include <iostream>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace client {

// Used to generate unique tags to link queries to responses.
uint64_t tag = 0;

types::Query CreateQuery(uint64_t value, uint32_t parties) {
  types::Query query;
  query.set_tag(tag++);
  query.set_tally(1);
  auto shares = primitives::GenerateIncrementalSecretShares(value, parties);
  for (const auto &share : shares) {
    types::QueryShare *query_share = query.add_shares();
    query_share->set_x(share.x);
    query_share->set_y(share.y);
  }
  return query;
}

void ReconstructResponse(const types::Response &response) {
  uint64_t share = response.shares(0);
  uint64_t output = primitives::AdditiveReconstruct(response.tally(), share);
  std::cout << "response: " << output << std::endl;
}

}  // namespace client
}  // namespace protocol
}  // namespace drivacy
