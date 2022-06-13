#include <cassert>
#include <iostream>
#include <utility>

#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/party/party.h"

namespace DPPIR {
namespace protocol {

// Regular Party.
Party::Party(party_id_t party_id, server_id_t server_id,
             config::Config&& config, Database&& db)
    : party_id_(party_id),
      server_id_(server_id),
      party_count_(config.party_count),
      server_count_(config.server_count),
      // Sockets.
      back_(onion::CipherSize(party_count_ - party_id)),
      next_(onion::CipherSize(party_count_ - party_id - 1)),
      // Configuration
      config_(std::move(config)),
      party_config_(config_.parties.at(party_id_)),
      server_config_(party_config_.servers.at(server_id_)),
      // Database.
      db_(std::move(db)),
      // Shuffler.
      lshuffler_(server_config_.local_seed),
      // Counts.
      noise_count_(0),
      input_count_(0),
      shuffled_count_(0),
      // Batches.
      noise_(),
      ciphers_(onion::CipherSize(party_count_ - party_id - 1),
               onion::CipherSize(party_count_ - party_id)),
      tags_(),
      queries_(),
      responses_(),
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
  assert(this->server_count_ == 1);
  // Primary keys.
  for (config::PartyConfig& party : this->config_.parties) {
    this->pkeys_.push_back(party.onion_pkey);
  }
  // Initialize sockets.
  this->back_.Initialize(this->server_config_.port);
  const auto& nextparty = this->config_.parties.at(this->party_id_ + 1);
  const auto& nextserver = nextparty.servers.at(this->server_id_);
  this->next_.Initialize(nextserver.ip, nextserver.port);
}

void Party::InitializeNoiseSamples() {
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

void Party::InitializeCounts() {
  // Read the count of incoming queries from previous party.
  this->input_count_ = this->back_.ReadCount();
  this->shuffled_count_ = this->input_count_ + this->noise_count_;

  // Send output count to next party.
  this->next_.SendCount(this->shuffled_count_);

  // Log information.
  std::cout << "Input: " << this->input_count_
            << "; Noise: " << this->noise_count_
            << "; Shuffled: " << this->shuffled_count_ << std::endl;
}

void Party::InitializeShuffler() {
  this->lshuffler_.Initialize(this->shuffled_count_);
}

void Party::InitializeNoiseQueries() {
  index_t idx = 0;
  for (key_t key = this->noise_start_; key < this->noise_end_; key++) {
    sample_t sample = this->noise_[key - this->noise_start_];
    // Make as many queries targeting row i as sampled.
    for (index_t end = idx + sample; idx < end; idx++) {
      // Shuffle noise query.
      index_t target = this->lshuffler_.Shuffle(idx);
      // Create the noise query at the target index directly.
      this->MakeNoiseQuery(key, &this->queries_[target]);
    }
  }
  this->noise_.Free();
  this->noise_state_.Free();
}

}  // namespace protocol
}  // namespace DPPIR
