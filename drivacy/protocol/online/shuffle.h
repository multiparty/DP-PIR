// Copyright 2020 multiparty.org

// This file implements an efficient coordination-free distributed shuffling
// based on knuth shuffle and shared seeds.
//
// The shuffling is in two phases:
// 1. First, a machine has a bunch of queries to shuffle, the machine
//    bucketizes them between the different machines (including itself) in
//    the same party.
// Machines exchange the queries as per the bucketization.
// 2. Second, a machine receives a bunch of queries from various other machines
//    (including itself), and shuffles them among themselves.
//
// Instead of each machine bucketizing independently, which introduces variance
// into how many queries every machine ends up with after shuffling, we rely on
// a shared seed between all the machines.
// The machines simulate a global shuffle over all messages, including ones a
// machine never encounters physically, and then bucketize consistently so that
// every machine ends up with the same number of queries.
// This simulation is done ahead of time at initialization, and only requires
// knowledge about the counts of the queries.
// Only the relevant pieces of this simulated shuffling is stored within the
// shuffler:
// 1. For every query this machine has in phase 1, we store which machine is
//    its bucket in "machine_ids_".
// 2. For every query this machine is meant to receive in phase 2, we store
//    its index in the global shuffling order in "query_indices_".
//
// Phase 1 is encoded in MachineOfNextQuery(), and phase 2 is encoded in
// ShuffleQuery().
//
// Finally, in addition to what is described above, the inverse mappings are
// also stored, so that corresponding responses can be de-shuffled according
// to the same order. The de-shuffling occurs in two phases mirroring those of
// shuffling. The first phase of the de-shuffling is encoded in
// MachineOfNextResponse() and the second phase is encoded in
// DeshuffleResponse().
//
// While queries resulting out of phase 1 can be sent to other machines before
// phase 1 is completed, because all the machines are part of the same party,
// queries out of phase 2 can only be sent after the entire shuffling is
// completed by the machine, since they are sent outside of the party, to avoid
// leaking information about the shuffling. The same applies to the two phases
// of de-shuffling responses.
//
// ShuffleQuery() and DeshuffleResponse() return true to indicate that the phase
// is complete. After which, the queries/responses can be accessed in their new
// order using NextQuery() and NextResponse().
//
// Additionally, NextQueryState() exposes the stored state of a response from
// its query. Within a batch (delimited by calls to Initialize()), for every
// fixed machine_id value m, the ith call to NextQueryState(m) returns state
// associated with the ith response received from that machine.

#ifndef DRIVACY_PROTOCOL_ONLINE_SHUFFLE_H_
#define DRIVACY_PROTOCOL_ONLINE_SHUFFLE_H_

#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

#include "drivacy/primitives/util.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {
namespace online {

class Shuffler {
 public:
  Shuffler(uint32_t party_id, uint32_t machine_id, uint32_t party_count,
           uint32_t parallelism);

  // Accessors.
  uint32_t batch_size() { return this->batch_size_; }

  // Initialize the shuffler to handle a new batch.
  // Initialize must be called for every machine_id, passing the input
  // batch size of that machine.
  // When all machine input batch sizes are specified, Initialize returns
  // true, indicating that initialization is complete.
  bool Initialize(uint32_t machine_id, uint32_t size);

  // After the shuffler is initialized with the various batch size at every
  // machine, this function should be called to simulate a global shuffle,
  // record the relevant portions of the shuffling and deshuffling orders,
  // for later use during the actual online (phase 2) shuffling.
  void PreShuffle();

  // Returns a vector (logically a map) from a machine_id to the count
  // of queries expected to be received from that machine.
  std::vector<uint32_t> IncomingQueriesCount();
  std::vector<uint32_t> OutgoingNoiseQueriesCount(uint32_t noise_size);

  // Determines the machine the next query is meant to be sent to.
  uint32_t MachineOfNextQuery(const types::QueryState &query_state);
  // Determines the machine the next response is meant to be sent to.
  uint32_t MachineOfNextResponse();

  // Shuffle Query into shuffler.
  bool ShuffleQuery(uint32_t machine_id, const types::Query &query);
  // Deshuffle the response using stored shuffling order.
  bool DeshuffleResponse(uint32_t machine_id, const types::Response &response);

  // Get the next query in the shuffled order.
  types::Query &NextQuery();
  // Get the next response in the de-shuffled order.
  types::Response &NextResponse();
  // Get the next query state in the shuffled order.
  types::QueryState &NextQueryState(uint32_t machine_id);

 private:
  // Configurations.
  uint32_t party_id_;
  uint32_t machine_id_;
  uint32_t parallelism_;
  // Batch size configurations.
  uint32_t init_counter_;
  std::vector<uint32_t> size_;
  uint32_t batch_size_;
  uint32_t total_size_;
  uint32_t shuffled_query_count_;
  uint32_t deshuffled_response_count_;
  // Seeded RNG.
  primitives::util::Generator generator_;
  // Current index of the next query or response to shuffle or deshuffle.
  uint32_t query_index_;
  uint32_t response_index_;
  // Shuffled queries and responses meant for this machine.
  // These vectors are filled in incrementally as queries and responses are
  // added in using ShuffleQuery() and DeshuffleResponse().
  // The queries vector contains queries that this machine was meant to shuffle
  // and send to the next party.
  // The response vector contains responses that this machine was meant to
  // process, and were shuffled by other machines of this party but now need
  // deshuffling, before sending them to another party.
  std::vector<types::Query> shuffled_queries_;
  std::vector<types::Response> deshuffled_responses_;
  // Maps storing relavent information from simulated shuffling.
  // Invariant: for any machines m and m',
  //            query_indices[m'] at machine m == query_order_[m] at machine m'.

  // query_machine_ids_[i] = machine-bucket of the ith query (phase 1).
  uint32_t query_machine_ids_index_;
  std::vector<uint32_t> query_machine_ids_;
  // response_machine_ids_[i] = machine-bucket of the ith response (phase 1).
  uint32_t response_machine_ids_index;
  std::vector<uint32_t> response_machine_ids_;
  // query_order_[m][i] = the relative order of query q among all queries
  //                      sent from this machine to machine m, where q is
  //                      the i-th query sent from this machine to m.
  std::vector<std::pair<size_t, std::vector<uint32_t>>> query_order_;
  // query_indices_[m][i] = index in shuffled_queries_ of the ith query received
  //                        from machine m (phase 2).
  std::vector<std::pair<size_t, std::vector<uint32_t>>> query_indices_;
  // response_indices_[m][i] = index in deshuffled_responses_ of the ith
  //                           response received from machine m (phase 2).
  std::vector<std::pair<size_t, std::vector<uint32_t>>> response_indices_;
  // query_states_[m][i] = the query state associated with the ith response to
  //                       be received from machine m (phase 2).
  std::vector<std::pair<size_t, std::vector<types::QueryState>>> query_states_;
};

}  // namespace online
}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_ONLINE_SHUFFLE_H_
