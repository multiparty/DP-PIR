// Copyright 2020 multiparty.org

#define STATISTICAL_SIGNIFICANCE 100000
#define UNIFORM_QUANTA 100

#include "drivacy/primitives/noise.h"

#include <cassert>
#include <iostream>

#include "drivacy/primitives/util.h"

// Average samples.
inline float Avg(uint32_t a) { return 1.0 * a / STATISTICAL_SIGNIFICANCE; }

// Tests.
int main(int argc, char *argv[]) {
  // Hope uniform is more or less uniform!
  std::vector<uint32_t> counts(UNIFORM_QUANTA, 0);
  for (uint32_t i = 0; i < STATISTICAL_SIGNIFICANCE; i++) {
    float r = drivacy::primitives::util::RandUniform();
    uint32_t index = static_cast<uint32_t>(r * UNIFORM_QUANTA);
    counts[index]++;
  }
  uint32_t max_diff = 0;
  for (uint32_t i = 0; i < counts.size(); i++) {
    for (uint32_t j = 0; j < counts.size(); j++) {
      uint32_t diff = counts[i] - counts[j];
      if (counts[i] < counts[j]) {
        diff = counts[j] - counts[i];
      }
      max_diff = max_diff > diff ? max_diff : diff;
    }
  }
  assert(Avg(max_diff) < 0.005);

  // Is laplace truely laplace?
  // Should at least be centered around floor(cutoff + 1)
  float span[5] = {3.333, 7.1, 10, 12, 20};
  float cutoff[5] = {5.0, 9.25, 19.5, 29.7, 40.9};
  for (uint32_t i = 0; i < 5; i++) {
    float s = span[i];
    float c = cutoff[i];
    uint64_t total = 0;
    for (uint32_t i = 0; i < STATISTICAL_SIGNIFICANCE; i++) {
      total += drivacy::primitives::SampleFromDistribution(s, c);
    }
    float estimate = std::floor(c) + 1;
    float avg = Avg(total);
    float diff = estimate - avg;
    std::cout << estimate << " ? " << avg << " === " << (estimate / avg)
              << std::endl;
    assert(-2 < diff && diff < 2);
  }

  return 0;
}
