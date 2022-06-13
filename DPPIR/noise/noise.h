// This file is responsible for sampling DP-noise according to given
// configurations.

#include <cstdint>
#include <utility>

#include "DPPIR/types/types.h"

#ifndef DPPIR_NOISE_NOISE_H_
#define DPPIR_NOISE_NOISE_H_

namespace DPPIR {
namespace noise {

double RandUniform();

// Define DPPIR_SAMPLE to use real noise.
// For our experiments, we leave this undefined to add the average/expected
// amount of noise to avoid having to average out over many runs for small
// databases.
// #define DPPIR_SAMPLE

class NoiseDistribution {
 public:
  NoiseDistribution(double epsilon, double delta);

  // Either sample real noise or return the mean.
#ifndef DPPIR_SAMPLE
  inline sample_t Sample() const { return this->cutoff_; }
#else
  inline sample_t Sample() const { return this->SampleReal(); }
#endif

 private:
  // if true, we wont add any noise.
  bool debug_;
  // Parameters for sampling our modified laplace noise.
  double span_;
  double cutoff_;
  // Real noise sampling.
  sample_t SampleReal() const;

  // Make test a friend to access span_ and cutoff_.
#ifdef DPPIR_NOISE_TEST
  friend bool TestLaplace();
#endif
};

std::pair<key_t, key_t> FindRange(server_id_t server_id,
                                  server_id_t servers_count, index_t db_size);

}  // namespace noise
}  // namespace DPPIR

#endif  // DPPIR_NOISE_NOISE_H_
