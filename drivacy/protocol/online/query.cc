// Copyright 2020 multiparty.org

// This file contains the query phase of the protocol.

#include "drivacy/protocol/online/query.h"

#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace query {

std::pair<types::Query, types::QueryState> ProcessQuery(
    const types::Query &query, const types::CommonReferenceMap &common_map) {
  // Find corresponding common reference shares.
  types::Tag tag = query.tag;
  const types::CommonReference &common = common_map.at(tag);

  // Use common reference to construct next query and find the preshare.
  uint32_t tally =
      primitives::IncrementalReconstruct(query.tally, common.incremental_share);
  return std::make_pair(types::Query{common.next_tag, tally}, common.preshare);
}

}  // namespace query
}  // namespace online
}  // namespace protocol
}  // namespace drivacy
