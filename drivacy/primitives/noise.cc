// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/primitives/noise.h"

#include <cmath>
#include <cstdlib>

#include "drivacy/primitives/util.h"

namespace drivacy {
namespace primitives {

uint32_t UpperBound(double span, double cutoff) {
  return static_cast<uint32_t>(std::floor(2 * cutoff)) + 1;
}

uint32_t SampleFromDistribution(double span, double cutoff) {
  // Sample a plain laplace.
  int sign = (util::RandUniform() < 0.5) ? -1 : 1;
  float u = util::RandUniform();
  float lap = sign * span * std::log(1 - 2 * abs(u - 0.5));
  // Truncate/clamp to within [-cutoff, cutoff]
  lap = lap < -1 * cutoff ? -1 * cutoff : lap;
  lap = lap > cutoff ? cutoff : lap;
  // Laplace-ish in (0, floor(cutoff * 2) + 1].
  return static_cast<uint32_t>(std::floor(lap + cutoff)) + 1;
}

}  // namespace primitives
}  // namespace drivacy
