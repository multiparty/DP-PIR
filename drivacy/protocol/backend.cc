// Copyright 2020 multiparty.org

// This file contains backend-specific portions of the protocol.
// The backend still executes the appropriate code from query and response
// on top of this code.

#include "drivacy/protocol/backend.h"

#include <cstdint>

#include "drivacy/primitives/additive.h"
#include "drivacy/primitives/crypto.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace backend {

types::Response QueryToResponse(const types::IncomingQuery &query,
                                const types::Configuration &config,
                                const types::Table &table) {
  uint32_t party_id = config.parties();
  // No nested onion cipher: we are at the backend, we got to the very inner
  // layer of the onion encryption.
  types::QueryShare query_share;
  unsigned char *ptr = reinterpret_cast<unsigned char *>(&query_share);
  primitives::crypto::SingleLayerOnionDecrypt(party_id, query.cipher(), config,
                                              ptr);

  // Reconstruct the query, and find the response.
  uint64_t tally = query.tally();
  uint64_t query_value =
      primitives::IncrementalReconstruct(tally, {query_share.x, query_share.y});
  uint64_t response_value = table.at(query_value);

  // Share response value using preshare from query.
  uint64_t response_tally =
      primitives::AdditiveReconstruct(response_value, query_share.preshare);

  return types::Response(response_tally);
}

}  // namespace backend
}  // namespace protocol
}  // namespace drivacy
