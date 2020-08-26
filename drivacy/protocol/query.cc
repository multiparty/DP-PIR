// Copyright 2020 multiparty.org

// This file contains the query phase of the protocol.

#include "drivacy/protocol/query.h"

#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace query {

types::Query ProcessQuery(const types::Query &query, types::PartyState *state) {
  uint64_t new_tag = state->tag++;

  // Find next tally.
  const types::QueryShare &share = query.shares(0);
  uint64_t next_tally =
      primitives::IncrementalReconstruct(query.tally(), {share.x(), share.y()});

  // Construct next query object.
  types::Query next_query;
  next_query.set_tag(new_tag);
  next_query.set_tally(next_tally);
  auto it = query.shares().cbegin();
  for (it++; it != query.shares().cend(); it++) {
    *next_query.add_shares() = *it;
  }
  auto preit = query.preshares().cbegin();
  for (preit++; preit != query.preshares().cend(); preit++) {
    next_query.add_preshares(*preit);
  }

  // Save query state.
  types::QueryState query_state{query.preshares(0), query.tag()};
  state->tag_to_query_state.insert({new_tag, query_state});

  return next_query;
}

}  // namespace query
}  // namespace protocol
}  // namespace drivacy
