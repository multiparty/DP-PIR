#include "DPPIR/shuffle/parallel_shuffle.h"

#include "DPPIR/shuffle/util.h"

namespace DPPIR {
namespace shuffle {

// ParallelShuffler.
ParallelShuffler::ParallelShuffler(server_id_t server_id,
                                   server_id_t server_count, int shared_seed)
    : shared_seed_(shared_seed),
      server_id_(server_id),
      server_count_(server_count),
      slice_size_(0),
      forward_map_(nullptr),
      backward_map_(nullptr),
      forward_idx_(0),
      backward_idx_(nullptr),
      from_count_(nullptr),
      prefixsum_from_count_(nullptr),
      to_count_(nullptr),
      to_noise_count_(nullptr) {}

void ParallelShuffler::Initialize(const index_t* server_counts,
                                  index_t noise_count) {
  this->forward_idx_ = 0;

  // Sum up all the counts.
  index_t total_count = 0;
  for (server_id_t id = 0; id < this->server_count_; id++) {
    total_count += server_counts[id];
  }

  // How many element this server should get.
  index_t per_server = total_count / this->server_count_;
  this->slice_size_ = per_server;
  if (this->server_id_ == this->server_count_ - 1) {
    this->slice_size_ = total_count - (this->server_count_ - 1) * per_server;
  }

  // Allocate space.
  this->backward_idx_ = std::make_unique<index_t[]>(this->server_count_);
  this->from_count_ = std::make_unique<index_t[]>(this->server_count_);
  this->prefixsum_from_count_ =
      std::make_unique<index_t[]>(this->server_count_);
  this->to_count_ = std::make_unique<index_t[]>(this->server_count_);
  this->to_noise_count_ = std::make_unique<index_t[]>(this->server_count_);
  this->forward_map_ =
      std::make_unique<server_id_t[]>(server_counts[this->server_id_]);
  this->backward_map_ =
      std::make_unique<std::unique_ptr<index_t[]>[]>(this->server_count_);

  // Initially, all indices/counts are 0.
  for (server_id_t sid = 0; sid < this->server_count_; sid++) {
    this->from_count_[sid] = 0;
    this->prefixsum_from_count_[sid] = 0;
    this->to_count_[sid] = 0;
    this->to_noise_count_[sid] = 0;
    this->backward_idx_[sid] = 0;
  }

  // Create global mapping of messages to servers (initially identity).
  auto map = std::make_unique<server_id_t[]>(total_count);
  for (server_id_t sid = 0; sid < this->server_count_; sid++) {
    index_t start = sid * per_server;
    index_t end = start + per_server;
    if (sid == this->server_count_ - 1) {
      end = total_count;
    }
    for (index_t i = start; i < end; i++) {
      map[i] = sid;
    }
  }

  // Shuffle mapping.
  util::seed(this->shared_seed_);
  util::shuffle(map.get(), total_count);

  // Find how many messages will be sent to/from this server.
  // Tirm down map to this server section to compute forward_map_.
  server_id_t source = 0;
  index_t start_idx = 0;
  for (index_t idx = 0; idx < total_count; idx++) {
    while (idx - start_idx >= server_counts[source]) {
      start_idx += server_counts[source];
      source++;
    }

    server_id_t target = map[idx];
    // count messages sent to this server.
    if (target == this->server_id_) {
      this->from_count_[source]++;
    }
    if (source == this->server_id_) {
      if (idx - start_idx < noise_count) {
        this->to_noise_count_[target]++;
      }
      this->to_count_[target]++;
      this->forward_map_[idx - start_idx] = target;
    }
  }

  // Allocate the exact space needed for backward_map_.
  for (server_id_t sid = 0; sid < this->server_count_; sid++) {
    index_t& count = this->to_count_[sid];
    this->backward_map_[sid] = std::make_unique<index_t[]>(count);
    count = 0;
  }

  // Fill in reverse mapping in backward_map_.
  for (index_t i = 0; i < server_counts[this->server_id_]; i++) {
    server_id_t target = this->forward_map_[i];
    this->backward_map_[target][this->to_count_[target]++] = i;
  }

  // Compute left prefix sum of from_count_.
  for (server_id_t sid = 0; sid < this->server_count_ - 1; sid++) {
    this->prefixsum_from_count_[sid + 1] =
        this->prefixsum_from_count_[sid] + this->from_count_[sid];
  }
}

// Shuffling/deshuffling.
server_id_t ParallelShuffler::ShuffleOne() {
  return this->forward_map_[this->forward_idx_++];
}
index_t ParallelShuffler::DeshuffleOne(server_id_t server) {
  return this->backward_map_[server][this->backward_idx_[server]++];
}

// How many messages will this server send to target server during shuffling.
index_t ParallelShuffler::CountToServer(server_id_t server) {
  return this->to_count_[server];
}
index_t ParallelShuffler::CountNoiseToServer(server_id_t server) {
  return this->to_noise_count_[server];
}

// How many messages will this server receive from server during shuffling.
index_t ParallelShuffler::CountFromServer(server_id_t server) {
  return this->from_count_[server];
}
index_t ParallelShuffler::PrefixSumCountFromServer(server_id_t server) {
  return this->prefixsum_from_count_[server];
}
server_id_t ParallelShuffler::FindSourceOf(index_t idx) {
  for (server_id_t id = 1; id < this->server_count_; id++) {
    if (this->prefixsum_from_count_[id] > idx) {
      return id - 1;
    }
  }
  return this->server_count_ - 1;
  /*
  server_id_t b = this->server_count_ - 1;
  server_id_t lo = 0;
  server_id_t hi = this->server_count_;
  while (lo < hi) {
    server_id_t mid = (hi + lo) >> 1;
    if (this->prefixsum_from_count_[mid] > idx) {
      hi = mid;
    } else if (mid < b && this->prefixsum_from_count_[mid + 1] < idx) {
      lo = mid;
    } else {
      return mid;
    }
  }
  assert(false);
  */
}

// Size of the output slice belonging to this server.
index_t ParallelShuffler::GetServerSliceSize() const {
  return this->slice_size_;
}

// Freeing memory.
void ParallelShuffler::FinishForward() { this->forward_map_ = nullptr; }
void ParallelShuffler::FinishBackward() {
  this->backward_map_ = nullptr;
  this->backward_idx_ = nullptr;
}

}  // namespace shuffle
}  // namespace DPPIR
