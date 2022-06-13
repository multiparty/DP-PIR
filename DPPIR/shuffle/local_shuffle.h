#ifndef DPPIR_SHUFFLE_LOCAL_SHUFFLE_H_
#define DPPIR_SHUFFLE_LOCAL_SHUFFLE_H_

#include <memory>

#include "DPPIR/types/types.h"

namespace DPPIR {
namespace shuffle {

// Local shuffler.
class LocalShuffler {
 public:
  explicit LocalShuffler(int local_seed);
  void Initialize(index_t local_count);

  // Shuffling/deshuffling.
  index_t Shuffle(index_t idx);
  index_t Deshuffle(index_t idx);

  // Clear maps.
  void FinishForward();
  void FinishBackward();

 private:
  int local_seed_;
  // Shuffling maps.
  std::unique_ptr<index_t[]> forward_map_;
  std::unique_ptr<index_t[]> backward_map_;
};

}  // namespace shuffle
}  // namespace DPPIR

#endif  // DPPIR_SHUFFLE_LOCAL_SHUFFLE_H_
