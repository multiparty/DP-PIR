// Copyright 2020 multiparty.org

// This file is responsible for sampling DP-noise according to given
// configurations.

#include <cstdint>

#ifndef DRIVACY_PRIMITIVES_NOISE_H_
#define DRIVACY_PRIMITIVES_NOISE_H_

namespace drivacy {
namespace primitives {

uint32_t UpperBound(double span, double cutoff);

uint32_t SampleFromDistribution(double span, double cutoff);

}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_NOISE_H_
