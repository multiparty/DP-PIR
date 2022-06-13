#define DPPIR_NOISE_TEST
#define DPPIR_SAMPLE

#include "DPPIR/noise/noise.h"

#include <cmath>
#include <iostream>
#include <unordered_map>

namespace DPPIR {
namespace noise {

#define STATISTICAL_SIGNIFICANCE 1000000
#define UNIFORM_QUANTA 1000

bool TestUniform() {
  std::cout << "Testing uniform with " << STATISTICAL_SIGNIFICANCE
            << " iterations and " << UNIFORM_QUANTA << " buckets " << std::endl;

  // Hope uniform is more or less uniform!
  std::unordered_map<int, int> counts;
  for (uint32_t i = 0; i < STATISTICAL_SIGNIFICANCE; i++) {
    double r = DPPIR::noise::RandUniform();
    int index = static_cast<int>(r * UNIFORM_QUANTA);
    counts[index]++;
  }

  // Every bucket should have roughly (samples / buckets) entries.
  int expected = STATISTICAL_SIGNIFICANCE / UNIFORM_QUANTA;
  int tolerance = expected * 0.15;
  for (auto [bucket, count] : counts) {
    if (std::abs(expected - count) > tolerance) {
      std::cout << "Uniform is not uniform!" << std::endl;
      std::cout << "Bucket " << bucket << " has " << count << " samples"
                << std::endl;
      std::cout << "Expected " << expected << " samples on average"
                << std::endl;
      return false;
    }
  }
  std::cout << "Test pass!" << std::endl;
  return true;
}

// Laplace testing cases.
const double EPSILON[2] = {0.1, 0.01};
const double DELTA[2] = {0.000001, 0.0000001};
const double SPAN[4] = {20, 20, 200, 200};
const double CUTOFF[4] = {276, 322, 2763, 3223};

bool TestLaplace() {
  // Is laplace truely laplace?
  // Should at least be centered around floor(cutoff + 1)
  for (size_t i = 0; i < 2; i++) {
    for (size_t j = 0; j < 2; j++) {
      // Find distribution paramters.
      double epsilon = EPSILON[i];
      double delta = DELTA[j];
      std::cout << "Test epsilon = " << epsilon << ", delta = " << delta
                << std::endl;

      // Create distribution instance.
      DPPIR::noise::NoiseDistribution distribution(epsilon, delta);

      // Check parameters.
      double span = SPAN[i * 2 + j];
      double cutoff = CUTOFF[i * 2 + j];
      if (distribution.span_ != span) {
        std::cout << "Expected span = " << span << std::endl;
        std::cout << "Instead found " << distribution.span_ << std::endl;
        return false;
      }
      if (std::floor(distribution.cutoff_) != cutoff) {
        std::cout << "Expected cutoff = " << cutoff << std::endl;
        std::cout << "Instead found " << distribution.cutoff_ << std::endl;
        return false;
      }

      // Sample plenty.
      uint64_t total = 0;
      for (size_t i = 0; i < STATISTICAL_SIGNIFICANCE; i++) {
        DPPIR::index_t sample = distribution.Sample();
        if (sample > std::floor(2 * distribution.cutoff_)) {
          std::cout << "Sample out of bounds!" << std::endl;
          return false;
        }
        total += sample;
      }

      // Must average around cutoff.
      double avg = 1.0 * total / STATISTICAL_SIGNIFICANCE;
      double estimate = std::floor(distribution.cutoff_);
      if (std::abs(avg - estimate) > 2) {
        std::cout << "Expected samples to average " << estimate << std::endl;
        std::cout << "Instead found " << avg << std::endl;
        return false;
      }
      std::cout << "Test pass!" << std::endl;
    }
  }
  return true;
}

}  // namespace noise
}  // namespace DPPIR

// Tests.
int main(int argc, char *argv[]) {
  // Ensure uniform is close to uniform.
  if (!DPPIR::noise::TestUniform()) {
    return 1;
  }

  // Test our noise modified laplace distribution.
  if (!DPPIR::noise::TestLaplace()) {
    return 1;
  }

  std::cout << "All done!" << std::endl;
  return 0;
}
