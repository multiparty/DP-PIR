// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/noise.h"

#include <cassert>
#include <cmath>
#include <cstdlib>

#include "drivacy/primitives/util.h"
#include "drivacy/protocol/client.h"

namespace drivacy {
namespace protocol {
namespace noise {

namespace {

std::pair<uint32_t, uint32_t> FindRange(uint32_t machine_id,
                                        uint32_t parallelism,
                                        uint32_t table_size) {
  assert(table_size > parallelism);
  uint32_t range_size = std::ceil(1.0 * table_size / parallelism);
  uint32_t range_start = (machine_id - 1) * range_size;
  uint32_t range_end = range_start + range_size;
  if (range_end > table_size) {
    range_end = table_size;
  }
  return std::make_pair(range_start, range_end);
}

uint32_t SampleFromDistribution(double span, double cutoff) {
  // Sample a plain laplace.
  int sign = (primitives::util::RandUniform() < 0.5) ? -1 : 1;
  float u = primitives::util::RandUniform();
  float lap = sign * span * std::log(1 - 2 * abs(u - 0.5));
  // Truncate/clamp to within [-cutoff, cutoff]
  lap = lap < -1 * cutoff ? -1 * cutoff : lap;
  lap = lap > cutoff ? cutoff : lap;
  // Laplace-ish in (0, floor(cutoff * 2) + 1].
  return static_cast<uint32_t>(std::floor(lap + cutoff)) + 1;
}

}  // namespace

std::pair<uint32_t, std::vector<uint32_t>> SampleNoise(uint32_t machine_id,
                                                       uint32_t parallelism,
                                                       uint32_t table_size,
                                                       double span,
                                                       double cutoff) {
  // Store all sampled noise queries in result.
  std::pair<int32_t, std::vector<uint32_t>> result =
      std::make_pair(0, std::vector<uint32_t>());
  if (span == 0.0) {
    return result;
  }
  // Only consider entries within our range.
  auto [start, end] = FindRange(machine_id, parallelism, table_size);
  // Sample noise for each table entry independently.
  for (uint32_t index = start; index < end; index++) {
    uint32_t count = SampleFromDistribution(span, cutoff);
    result.first += count;
    result.second.push_back(count);
  }
  return result;
}

std::vector<types::OutgoingQuery> MakeNoisyQueries(
    uint32_t party_id, uint32_t machine_id, const types::Configuration &config,
    const types::Table &table, const std::vector<uint32_t> &counts) {
  std::vector<types::OutgoingQuery> result;
  // Only consider entries within our range.
  auto [start, end] = FindRange(machine_id, config.parallelism(), table.size());
  uint32_t index = 0;
  // Sample noise for each table entry independently.
  for (const auto &[query, _] : table) {
    if (start <= index && index < end) {
      uint32_t count = counts[index - start];
      for (uint32_t i = 0; i < count; i++) {
        result.push_back(client::CreateQuery(query, config, party_id));
      }
    }
    index++;
  }
  return result;
}

}  // namespace noise
}  // namespace protocol
}  // namespace drivacy
