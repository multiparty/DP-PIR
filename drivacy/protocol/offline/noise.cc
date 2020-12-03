// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/offline/noise.h"

#include <cmath>
#include <cstdlib>

#include "drivacy/primitives/noise.h"
#include "drivacy/protocol/offline/client.h"

namespace drivacy {
namespace protocol {
namespace offline {
namespace noise {

std::vector<uint32_t> SampleNoiseHistogram(uint32_t machine_id,
                                           uint32_t parallelism,
                                           uint32_t table_size, uint32_t span,
                                           uint32_t cutoff) {
  std::vector<uint32_t> result;
  // Only consider entries within our range.
  auto [start, end] =
      primitives::FindRange(machine_id, parallelism, table_size);
  for (uint32_t i = start; i < end; i++) {
    result.push_back(primitives::SampleFromDistribution(span, cutoff));
  }
  return result;
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
