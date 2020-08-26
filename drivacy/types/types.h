// Copyright 2020 multiparty.org

// Helpers for file and file protobuf IO.

#ifndef DRIVACY_TYPES_TYPES_H_
#define DRIVACY_TYPES_TYPES_H_

#include <cstdlib>
#include <unordered_map>

namespace drivacy {
namespace types {

using Table = std::unordered_map<uint64_t, uint64_t>;

struct IncrementalSecretShare {
  uint64_t x;  // The additive component.
  uint64_t y;  // The multiplicative component.
};

}  // namespace types
}  // namespace drivacy

#endif  // DRIVACY_TYPES_TYPES_H_
