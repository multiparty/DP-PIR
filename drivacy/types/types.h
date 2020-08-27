// Copyright 2020 multiparty.org

// Helpers for file and file protobuf IO.

#ifndef DRIVACY_TYPES_TYPES_H_
#define DRIVACY_TYPES_TYPES_H_

#include <cstdint>
#include <unordered_map>

namespace drivacy {
namespace types {

using Table = std::unordered_map<uint64_t, uint64_t>;

struct IncrementalSecretShare {
  uint64_t x;  // The additive component.
  uint64_t y;  // The multiplicative component.
};

// The state to keep per query.
struct QueryState {
  uint64_t preshare;  // The preshare for use while responding to the query.
  uint64_t tag;       // The tag that was associated with this query.
};

// The struct representing the plain text single share in a query.
// This is encrypted into bytes and decrypted from bytes in the crypto
// wrapper.
struct QueryShare {
  uint64_t x;
  uint64_t y;
  uint64_t preshare;
};

// A party state. This survives between a query and its response.
struct PartyState {
  uint64_t party_id;
  uint64_t tag;
  std::unordered_map<uint64_t, QueryState> tag_to_query_state;
  PartyState(uint64_t party_id) : party_id(party_id) { tag = 0; }
};

}  // namespace types
}  // namespace drivacy

#endif  // DRIVACY_TYPES_TYPES_H_
