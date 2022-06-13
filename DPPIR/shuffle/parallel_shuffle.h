#ifndef DPPIR_SHUFFLE_PARALLEL_SHUFFLE_H_
#define DPPIR_SHUFFLE_PARALLEL_SHUFFLE_H_

#include <memory>

#include "DPPIR/types/types.h"

namespace DPPIR {
namespace shuffle {

// Parallel shuffler.
class ParallelShuffler {
 public:
  ParallelShuffler(server_id_t server_id, server_id_t server_count,
                   int shared_seed);
  void Initialize(const index_t* server_counts, index_t noise_count);

  // Shuffling/deshuffling.
  server_id_t ShuffleOne();
  index_t DeshuffleOne(server_id_t server);

  // Counts.
  // How many messages will this server send to target server during shuffling.
  index_t CountToServer(server_id_t server);
  index_t CountNoiseToServer(server_id_t server);
  // How many messages will this server receive from server during shuffling.
  index_t CountFromServer(server_id_t server);
  index_t PrefixSumCountFromServer(server_id_t server);
  server_id_t FindSourceOf(index_t idx);

  // Size of the output slice belonging to this server.
  index_t GetServerSliceSize() const;

  // Freeing memory.
  void FinishForward();
  void FinishBackward();

 private:
  int shared_seed_;
  server_id_t server_id_;
  server_id_t server_count_;
  index_t slice_size_;
  // Shuffling maps.
  std::unique_ptr<server_id_t[]> forward_map_;
  std::unique_ptr<std::unique_ptr<index_t[]>[]> backward_map_;
  // Counters.
  index_t forward_idx_;
  std::unique_ptr<index_t[]> backward_idx_;
  // Counts of sent/received messages per server.
  std::unique_ptr<index_t[]> from_count_;
  std::unique_ptr<index_t[]> prefixsum_from_count_;
  std::unique_ptr<index_t[]> to_count_;
  std::unique_ptr<index_t[]> to_noise_count_;
};

}  // namespace shuffle
}  // namespace DPPIR

#endif  // DPPIR_SHUFFLE_PARALLEL_SHUFFLE_H_
