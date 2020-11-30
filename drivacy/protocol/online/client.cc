// Copyright 2020 multiparty.org

// This file contains the client-side portion of the protocol, including
// creating a query and reconstructing the response.

#include "drivacy/protocol/online/client.h"

#include <vector>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace client {

types::Query CreateQuery(uint64_t value,
                         const std::vector<types::Message> &common) {
  return CreateQuery(value, common, 0);
}

types::Query CreateQuery(uint64_t value,
                         const std::vector<types::Message> &common,
                         uint32_t party_id) {
  // Share query value and create additive preshares of the response.
  types::Tag tag = common.at(0).tag;
  uint32_t tally = primitives::GenerateIncrementalSecretShares(value, common);
  return {tag, tally};
}

uint64_t ReconstructResponse(const types::Response &response,
                             const types::QueryState &preshare) {
  return primitives::AdditiveReconstruct(response, preshare);
}

}  // namespace client
}  // namespace online
}  // namespace protocol
}  // namespace drivacy
