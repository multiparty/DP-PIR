#include "DPPIR/noise/noise.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>

namespace DPPIR {
namespace noise {

// Laplace helpers.
namespace {

double Laplace(double mean, double span) {
  int sign = (RandUniform() < 0.5) ? -1 : 1;
  double u = RandUniform();
  return mean - sign * span * std::log(1 - 2 * std::abs(u - 0.5));
}

// Returns the x such that Prob[lap(mean, span) <= x] = prob.
double InvCDF(double mean, double span, double prob) {
  int sign = (prob > 0.5) ? -1 : 1;
  return mean - sign * span * std::log(1 - 2 * std::abs(prob - 0.5));
}

}  // namespace

// Uniform in [0, 1)
double RandUniform() { return std::rand() / static_cast<double>(RAND_MAX); }

// Find the range that the server is responsible for adding noise for.
std::pair<key_t, key_t> FindRange(server_id_t server_id,
                                  server_id_t servers_count, index_t db_size) {
  assert(db_size >= servers_count);
  index_t range_size = std::ceil(1.0 * db_size / servers_count);
  key_t range_start = server_id * range_size;
  key_t range_end = range_start + range_size;
  if (server_id == servers_count - 1) {
    range_end = db_size;
  }
  return std::make_pair(range_start, range_end);
}

// NoiseDistribution.
NoiseDistribution::NoiseDistribution(double epsilon, double delta) {
  if (epsilon == 0 || delta == 0) {
    this->debug_ = true;
    this->span_ = 0;
    this->cutoff_ = 0;
    std::cout << "No noise!" << std::endl;
  } else {
    this->debug_ = false;
    this->span_ = 2 / epsilon;
    this->cutoff_ = InvCDF(0, this->span_, delta / 2);
    std::cout << "Noise cutoff: " << this->cutoff_ << std::endl;
  }
  // Noise domain must fit inside sample_t.
  index_t max = std::floor(2 * this->cutoff_);
  if (max != static_cast<sample_t>(max)) {
    std::cout << "sample_t too small to fit noise, max = " << max << std::endl;
    assert(false);
  }
}

sample_t NoiseDistribution::SampleReal() const {
  // Smaple noise from regular laplace, then clamp and floor.
  double u = Laplace(0, this->span_);
  u = std::fmax(0, this->cutoff_ + std::fmin(this->cutoff_, u));
  return std::floor(u);
}

}  // namespace noise
}  // namespace DPPIR
