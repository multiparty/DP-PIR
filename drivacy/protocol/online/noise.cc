// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/online/noise.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "drivacy/primitives/noise.h"
#include "drivacy/protocol/offline/noise.h"
#include "drivacy/protocol/online/client.h"
#include "drivacy/util/fake.h"

namespace drivacy {
namespace protocol {
namespace online {
namespace noise {

std::vector<uint32_t> SampleNoiseHistogram(uint32_t machine_id,
                                           uint32_t parallelism,
                                           uint32_t table_size, uint32_t span,
                                           uint32_t cutoff) {
  return offline::noise::SampleNoiseHistogram(machine_id, parallelism,
                                              table_size, span, cutoff);
}

std::vector<types::Query> MakeNoiseQueriesFromHistogram(
    uint32_t party_id, uint32_t machine_id, uint32_t party_count,
    uint32_t parallelism, const types::Table &table,
    const std::vector<uint32_t> &noise_histogram,
    const std::vector<std::vector<types::Message>> &commons) {
  std::vector<types::Query> result;
  // Only consider entries within our range.
  auto [start, end] =
      primitives::FindRange(machine_id, parallelism, table.size());
  assert(noise_histogram.size() == end - start);
  uint32_t index = 0;
  for (const auto &[query, _] : table) {
    if (start <= index && index < end) {
      uint32_t count = noise_histogram.at(index - start);
      for (uint32_t i = 0; i < count; i++) {
        result.push_back(client::CreateQuery(
            query,  // commons.at(index - start), party_id));
            fake::FakeIt(party_count - party_id), party_id));
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
