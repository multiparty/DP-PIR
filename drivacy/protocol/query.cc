// Copyright 2020 multiparty.org

// This file contains the query phase of the protocol.

#include "drivacy/protocol/query.h"

#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace query {

types::Query ProcessQuery(const types::Query &query) {
  // Find next tally.
  const types::QueryShare &share = query.shares(0);
  uint64_t next_tally =
      primitives::IncrementalReconstruct(query.tally(), {share.x(), share.y()});

  // Construct next query object.
  types::Query next_query;
  next_query.set_tag(query.tag());
  next_query.set_tally(next_tally);
  auto iterator = query.shares().cbegin();
  for (iterator++; iterator != query.shares().cend(); iterator++) {
    *next_query.add_shares() = *iterator;
  }

  return next_query;
}

}  // namespace query
}  // namespace protocol
}  // namespace drivacy
