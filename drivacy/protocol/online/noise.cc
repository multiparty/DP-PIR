// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/online/noise.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "drivacy/primitives/noise.h"
#include "drivacy/protocol/online/client.h"

namespace drivacy {
namespace protocol {
namespace online {
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

}  // namespace

std::vector<types::Query> MakeNoisyQueries(
    uint32_t party_id, uint32_t machine_id, uint32_t parallelism,
    const types::Table &table, double span, double cutoff,
    const std::vector<std::vector<types::Message>> &commons) {
  std::vector<types::Query> result;
  // Only consider entries within our range.
  auto [start, end] = FindRange(machine_id, parallelism, table.size());
  uint32_t index = 0;
  // Sample noise for each table entry independently.
  for (const auto &[query, _] : table) {
    if (start <= index && index < end) {
      uint32_t count = primitives::SampleFromDistribution(span, cutoff);
      for (uint32_t i = 0; i < count; i++) {
        result.push_back(
            client::CreateQuery(query, commons.at(index - start), party_id));
      }
    }
    index++;
  }
  return result;
}

}  // namespace noise
}  // namespace online
}  // namespace protocol
}  // namespace drivacy
