// Copyright 2020 multiparty.org

// This file implements an efficient shuffling functionality
// based on knuth shuffle.
// The shuffler is incremental: it shuffles the queries as they come.
// The shuffler remembers the generated order, which can be used to
// de-shuffle responses as they come.

#ifndef DRIVACY_PROTOCOL_SHUFFLE_H_
#define DRIVACY_PROTOCOL_SHUFFLE_H_

#include <unordered_map>

#include "drivacy/types/types.h"

namespace drivacy {
namespace protocol {

class Shuffler {
 public:
  Shuffler() {}

  // Initialize the shuffler to handle a new batch of the given size.
  void Initialize(uint32_t size);

  // Shuffle Query into shuffler.
  bool ShuffleQuery(types::OutgoingQuery query);

  // Deshuffle the response using stored shuffling order.
  bool DeshuffleResponse(types::Response response);

  // Get the next query in the shuffled order.
  types::OutgoingQuery NextQuery();

  // Get the next query state in the shuffled order.
  types::QueryState NextQueryState();

  // Get the next response in the de-shuffled order.
  types::Response NextResponse();

 private:
  // Stores the queries after they are shuffled.
  std::unordered_map<uint32_t, types::OutgoingQuery> shuffled_queries_;
  // Stores the responses after they have been de-shuffled.
  std::unordered_map<uint32_t, types::Response> deshuffled_responses_;
  // Remembers the inverese shuffling order: maps i to j if shuffled_queries[i]
  // was the jth query originally.
  std::unordered_map<uint32_t, uint32_t> shuffling_order_;
  // Used to remember elements to swap (because of shuffling) that we could
  // not swap at the time because one of them was not available.
  // When that element is available, this map is checked to complete the swap.
  // Used only during shuffling and then freed.
  std::unordered_map<uint32_t, uint32_t> shuffling_swaps_;
  // Current index of the next query or response to shuffle or deshuffle.
  uint32_t query_index_;
  uint32_t response_index_;
  // Total size of queries or responses.
  uint32_t size_;
};

}  // namespace protocol
}  // namespace drivacy

#endif  // DRIVACY_PROTOCOL_SHUFFLE_H_
