// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/protocol/noise.h"

#include "drivacy/protocol/client.h"

namespace drivacy {
namespace protocol {
namespace noise {

namespace {

uint32_t SampleFromDistribution(double span, double cutoff) { return 1; }

}  // namespace

std::list<types::OutgoingQuery> SampleNoise(uint32_t party_id,
                                            uint32_t machine_id,
                                            const types::Configuration &config,
                                            const types::Table &table,
                                            double span, double cutoff) {
  // Store all sampled noise queries in result.
  std::list<types::OutgoingQuery> result;
  if (span == 0.0) {
    return result;
  }

  // Sample noise for each table entry independently.
  for (const auto &[query, _] : table) {
    uint32_t count = SampleFromDistribution(span, cutoff);
    for (uint32_t i = 0; i < count; i++) {
      result.push_back(client::CreateQuery(query, config, party_id));
    }
  }
  return result;
}

}  // namespace noise
}  // namespace protocol
}  // namespace drivacy
