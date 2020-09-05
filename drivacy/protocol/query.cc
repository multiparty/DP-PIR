// Copyright 2020 multiparty.org

// This file contains the query phase of the protocol.

#include "drivacy/protocol/query.h"

#include <cstdint>

#include "drivacy/primitives/crypto.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace query {

uint32_t QuerySize(uint32_t party_id, uint32_t party_count) {
  return sizeof(uint32_t) +
         primitives::crypto::OnionCipherSize(party_id, party_count);
}

types::Query ProcessQuery(const types::Query &query,
                          const types::Configuration &config,
                          types::PartyState *state) {
  uint64_t new_tag = state->tag++;

  // Construct next query object.
  types::Query next_query;
  next_query.set_tag(new_tag);

  // Onion decrypt.
  types::QueryShare share = primitives::crypto::SingleLayerOnionDecrypt(
      state->party_id, query, config, &next_query);

  // Compute the next tally.
  uint64_t next_tally =
      primitives::IncrementalReconstruct(query.tally(), {share.x, share.y});
  next_query.set_tally(next_tally);

  // Save query state.
  types::QueryState query_state{share.preshare, query.tag()};
  state->tag_to_query_state.insert({new_tag, query_state});

  return next_query;
}

}  // namespace query
}  // namespace protocol
}  // namespace drivacy
