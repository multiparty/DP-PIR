// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/offline/noise.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <utility>

#include "drivacy/primitives/noise.h"
#include "drivacy/protocol/offline/client.h"

namespace drivacy {
namespace protocol {
namespace offline {
namespace noise {

// Upper bound on how much noise we might generate.
uint64_t UpperBound(uint32_t machine_id, uint32_t parallelism,
                    uint32_t table_size, double span, double cutoff) {
  if (span == 0) {
    return 0;
  }

  uint32_t range_size = std::ceil(1.0 * table_size / parallelism);
  uint32_t last_range_size = table_size - (machine_id - 1) * range_size;
  range_size = range_size < last_range_size ? range_size : last_range_size;
  return primitives::UpperBound(span, cutoff) * range_size;
}

std::vector<std::vector<types::Message>> CommonReferenceForNoise(
    uint32_t party_id, uint32_t party_count, uint32_t count, uint32_t seed) {
  std::vector<std::vector<types::Message>> result;
  result.reserve(count);
  for (uint32_t i = 0; i < count; i++) {
    result.push_back(std::move(
        client::SampleCommonReference(seed++, party_id, party_count)));
  }
  return result;
}

}  // namespace noise
}  // namespace offline
}  // namespace protocol
}  // namespace drivacy
