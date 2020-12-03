// Copyright 2020 multiparty.org

// This file is responsible for sampling DP-noise according to given
// configurations.

#include <cstdint>
#include <utility>

#ifndef DRIVACY_PRIMITIVES_NOISE_H_
#define DRIVACY_PRIMITIVES_NOISE_H_

namespace drivacy {
namespace primitives {

std::pair<uint32_t, uint32_t> FindRange(uint32_t machine_id,
                                        uint32_t parallelism,
                                        uint32_t table_size);

uint32_t SampleFromDistribution(double span, double cutoff);

}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_NOISE_H_
