// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/response.h"

#include "drivacy/primitives/additive.h"

namespace drivacy {
namespace protocol {
namespace response {

types::Response ProcessResponse(const types::Response &response,
                                types::PartyState *state) {
  const types::QueryState &query_state =
      state->tag_to_query_state.at(response.tag());

  // Find next tally.
  uint64_t next_tally =
      primitives::AdditiveReconstruct(response.tally(), query_state.preshare);

  // Construct next response object.
  types::Response next_response;
  next_response.set_tag(query_state.tag);
  next_response.set_tally(next_tally);

  // Free up state.
  state->tag_to_query_state.erase(response.tag());

  return next_response;
}

}  // namespace response
}  // namespace protocol
}  // namespace drivacy
