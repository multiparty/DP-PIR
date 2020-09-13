// Copyright 2020 multiparty.org

// This file contains the query phase of the protocol.

#include "drivacy/protocol/query.h"

#include "drivacy/primitives/crypto.h"
#include "drivacy/primitives/incremental.h"

namespace drivacy {
namespace protocol {
namespace query {

types::OutgoingQuery ProcessQuery(uint32_t party_id,
                                  const types::IncomingQuery &query,
                                  const types::Configuration &config) {
  // Allocate memory for outgoing query.
  types::OutgoingQuery outgoing_query(party_id, config.parties());

  // Onion decrypt this party's share and next parties onion cipher.
  primitives::crypto::SingleLayerOnionDecrypt(party_id, query.cipher(), config,
                                              outgoing_query.buffer());

  // Compute the next tally.
  uint64_t tally = query.tally();
  types::QueryShare share = outgoing_query.share();
  outgoing_query.set_tally(
      primitives::IncrementalReconstruct(tally, {share.x, share.y}));

  // Store preshare.
  outgoing_query.set_preshare(share.preshare);

  return outgoing_query;
}

}  // namespace query
}  // namespace protocol
}  // namespace drivacy
