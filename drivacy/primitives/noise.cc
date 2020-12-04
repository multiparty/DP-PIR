// Copyright 2020 multiparty.org

// This file contains the response phase of the protocol.

#include "drivacy/primitives/noise.h"

#include <cmath>
#include <cstdlib>

#include "drivacy/primitives/util.h"

namespace drivacy {
namespace primitives {

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
  if (span == 0) {
    return 0;
  }
  // Sample a plain laplace.
  int sign = (util::RandUniform() < 0.5) ? -1 : 1;
  float u = util::RandUniform();
  float lap = sign * span * std::log(1 - 2 * abs(u - 0.5));
  // Truncate/clamp to within [-cutoff, cutoff]
  lap = lap < -1 * cutoff ? -1 * cutoff : lap;
  lap = lap > cutoff ? cutoff : lap;
  // Laplace-ish in [0, floor(cutoff * 2)].
  // TODO(babman): below is the real value, but for experiments we use the mean
  // to avoid having to average out over many experiments.
  // return static_cast<uint32_t>(std::floor(lap + cutoff));
  return static_cast<uint32_t>(std::floor(cutoff));
}

}  // namespace primitives
}  // namespace drivacy
