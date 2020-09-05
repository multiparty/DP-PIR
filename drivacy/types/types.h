// Copyright 2020 multiparty.org

// C++ types used in the protocol.

#ifndef DRIVACY_TYPES_TYPES_H_
#define DRIVACY_TYPES_TYPES_H_

#include <cstdint>
#include <list>
#include <unordered_map>

namespace drivacy {
namespace types {

using Table = std::unordered_map<uint64_t, uint64_t>;

struct IncrementalSecretShare {
  uint64_t x;  // The additive component.
  uint64_t y;  // The multiplicative component.
};

// The struct representing the plain text single share in a query.
// This is encrypted into bytes and decrypted from bytes in the crypto
// wrapper.
struct QueryShare {
  uint64_t x;
  uint64_t y;
  uint64_t preshare;
};

// The state to keep per query.
struct QueryState {
  // The preshare for use while responding to the query.
  uint64_t preshare;
  // The original index of the corresponding query prior to shuffle.
  uint32_t index;
};

// A placeholder for storing a processed query.
struct QueryHolder {
  uint64_t &tally;
  uint64_t &preshare;
  unsigned char *shares;
  // Constructor initializes references and pointer, later
  // the values are assigned into these references.
  QueryHolder(uint64_t &tally, uint64_t &preshare, unsigned char *shares)
      : tally(tally), preshare(preshare), shares(shares) {}
}

// A placeholder for storing a processed response.
struct ResponseHolder {
  uint64_t &tally;
  ResponseHolder(uint64_t &tally) : tally(tally) {}
}

// A client state. This survives between a query and its response.
struct ClientState {
  // A copy of previously made queries for sanity checks.
  std::list<uint64_t> queries;
  // The corresponding stored preshare (matches queries by index).
  std::list<uint64_t> preshares;
};

}  // namespace types
}  // namespace drivacy

#endif  // DRIVACY_TYPES_TYPES_H_
