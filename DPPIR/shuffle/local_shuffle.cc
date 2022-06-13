#include "DPPIR/shuffle/local_shuffle.h"

#include "DPPIR/shuffle/util.h"

namespace DPPIR {
namespace shuffle {

// Local shuffler.
LocalShuffler::LocalShuffler(int local_seed)
    : local_seed_(local_seed), forward_map_(nullptr), backward_map_(nullptr) {}

void LocalShuffler::Initialize(index_t local_count) {
  // Seed the random number generator.
  util::seed(this->local_seed_);

  // Create local mapping.
  this->forward_map_ = std::make_unique<index_t[]>(local_count);
  for (index_t i = 0; i < local_count; i++) {
    this->forward_map_[i] = i;
  }

  // Shuffle.
  util::shuffle(this->forward_map_.get(), local_count);

  // Create reverse mapping.
  this->backward_map_ = std::make_unique<index_t[]>(local_count);
  for (index_t i = 0; i < local_count; i++) {
    this->backward_map_[this->forward_map_[i]] = i;
  }
}

// Online Shuffling.
index_t LocalShuffler::Shuffle(index_t idx) { return this->forward_map_[idx]; }
index_t LocalShuffler::Deshuffle(index_t idx) {
  return this->backward_map_[idx];
}

// Clear maps.
void LocalShuffler::FinishForward() { this->forward_map_ = nullptr; }
void LocalShuffler::FinishBackward() { this->backward_map_ = nullptr; }

}  // namespace shuffle
}  // namespace DPPIR
