#include <cassert>
#include <iostream>
#include <utility>

#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/parallel_party/parallel_party.h"

namespace DPPIR {
namespace protocol {

// Constructor.
ParallelParty::ParallelParty(party_id_t party_id, server_id_t server_id,
                             config::Config&& config, Database&& db)
    : party_id_(party_id),
      server_id_(server_id),
      party_count_(config.party_count),
      server_count_(config.server_count),
      // Offline onion cipher sizes.
      input_cipher_size_(onion::CipherSize(party_count_ - party_id)),
      output_cipher_size_(onion::CipherSize(party_count_ - party_id - 1)),
      // Sockets.
      back_(input_cipher_size_),
      next_(output_cipher_size_),
      siblings_(server_id_, server_count_, output_cipher_size_),
      // Parallel count maps.
      at_sibling_counts_(server_count_, server_count_ + 1, 0),
      total_batch_size_(0),
      noise_from_sibling_counts_(server_count_, server_count_ + 1, 0),
      noise_from_sibling_prefixsum_(server_count_, server_count_ + 1, 0),
      received_from_sibling_counts_(server_count_, server_count_ + 1, 0),
      // Configuration
      config_(std::move(config)),
      party_config_(config_.parties.at(party_id_)),
      server_config_(party_config_.servers.at(server_id_)),
      // Database.
      db_(std::move(db)),
      // Shuffler.
      pshuffler_(server_id_, server_count_, party_config_.shared_seed),
      lshuffler_(server_config_.local_seed),
      // Counts.
      noise_count_(0),
      input_count_(0),
      shuffled_count_(0),
      // Batch data.
      noise_(),
      in_ciphers_(output_cipher_size_, input_cipher_size_),
      out_ciphers_(output_cipher_size_, input_cipher_size_),
      in_tags_(),
      in_queries_(),
      out_queries_(),
      in_responses_(),
      out_responses_(),
      // Offline state.
      queries_state_(),
      noise_state_(),
      // Noise distribution.
      distribution_(config_.epsilon, config_.delta),
      noise_start_(0),
      noise_end_(0),
      // Onion encryption keys.
      pkeys_() {
  assert(this->party_count_ >= 2 && this->party_id_ < this->party_count_ - 1);
  assert(this->server_count_ > 1 && this->server_id_ < this->server_count_);
  // Primary keys.
  for (config::PartyConfig& party : this->config_.parties) {
    this->pkeys_.push_back(party.onion_pkey);
  }
  // Initialize sockets.
  this->back_.Initialize(this->server_config_.port);
  const auto& nextparty = this->config_.parties.at(this->party_id_ + 1);
  const auto& nextserver = nextparty.servers.at(this->server_id_);
  this->next_.Initialize(nextserver.ip, nextserver.port);
  this->siblings_.Initialize(this->party_config_);
}

/*
 * Initialization steps: these steps should be done offline.
 * These initialize the various datastructures needed by the offline
 * and online stages.
 */

// Sample the noise (but do not make any noise queries yet).
// Also computes the total noise queries to be added by this server.
void ParallelParty::InitializeNoiseSamples() {
  this->noise_count_ = 0;
  // Compute the noise domain.
  auto pair =
      noise::FindRange(this->server_id_, this->server_count_, this->db_.Size());
  this->noise_start_ = pair.first;
  this->noise_end_ = pair.second;
  // Sample the noise per element in domain.
  key_t size = this->noise_end_ - this->noise_start_;
  this->noise_.Initialize(size);
  for (key_t i = 0; i < size; i++) {
    sample_t sample = this->distribution_.Sample();
    this->noise_.PushBack(sample);
    this->noise_count_ += sample;
  }
}

// Initialize the required datastructures and compute the appropriate counts
// for this batch.
// This is responsible for reading the input count from the previous party,
// exchanging counts with other sibling parallel servers.
void ParallelParty::InitializeCounts() {
  // Read the count of incoming queries from previous party.
  this->input_count_ = this->back_.ReadCount();

  // Broadcast incoming query count to all parallel servers.
  this->siblings_.BroadcastCount(this->noise_count_ + this->input_count_);

  // This is used to compute the count of all queries in the entire batch.
  this->total_batch_size_ = 0;
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->at_sibling_counts_[id] = this->siblings_.ReadCount(id);
    } else {
      this->at_sibling_counts_[id] = this->noise_count_ + this->input_count_;
    }
    this->total_batch_size_ += this->at_sibling_counts_[id];
  }

  // We can compute final count after shuffling sent out from this server, since
  // shuffling is guaranteed to produce uniform load for every sibling.
  this->shuffled_count_ = this->total_batch_size_ / this->server_count_;
  if (this->server_id_ == this->server_count_ - 1) {
    this->shuffled_count_ =
        this->total_batch_size_ - (this->shuffled_count_ * this->server_id_);
  }

  // Send count to next party so they can start initialization as well.
  this->next_.SendCount(this->shuffled_count_);

  // Log information.
  std::cout << "Input: " << this->input_count_
            << "; Noise: " << this->noise_count_
            << "; Shuffled: " << this->shuffled_count_ << std::endl;
}

// After all counts have been determined, initialize the shufflers.
void ParallelParty::InitializeShufflers() {
  // Initialize parallel shuffler.
  this->pshuffler_.Initialize(this->at_sibling_counts_.Ptr(),
                              this->noise_count_);

  // Sanity check.
  assert(this->shuffled_count_ == this->pshuffler_.GetServerSliceSize());

  // Give the count of noise queries we will sent to each sibling.
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->siblings_.SendCount(id, this->pshuffler_.CountNoiseToServer(id));
    }
  }

  // Read how many noise queries we will receive from each server.
  index_t noise_from_siblings_total = 0;
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->noise_from_sibling_counts_[id] = this->siblings_.ReadCount(id);
    } else {
      this->noise_from_sibling_counts_[id] =
          this->pshuffler_.CountNoiseToServer(id);
    }
    noise_from_siblings_total += this->noise_from_sibling_counts_[id];
    this->noise_from_sibling_prefixsum_[id] = noise_from_siblings_total;
  }

  // Initialize local shuffler.
  this->lshuffler_.Initialize(this->shuffled_count_);
}

// Make noise queries according to the samples.
void ParallelParty::InitializeNoiseQueries() {
  index_t idx = 0;
  for (key_t key = this->noise_start_; key < this->noise_end_; key++) {
    sample_t sample = this->noise_[key - this->noise_start_];
    // Make as many queries targeting row i as sampled.
    for (index_t end = idx + sample; idx < end; idx++) {
      // Create the noise query at the target index directly.
      this->MakeNoiseQuery(key, &this->in_queries_[idx]);
    }
  }
  this->noise_.Free();
  this->noise_state_.Free();
}

}  // namespace protocol
}  // namespace DPPIR
