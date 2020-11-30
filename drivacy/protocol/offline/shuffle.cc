// Copyright 2020 multiparty.org

// This file implements an efficient shuffling functionality
// based on knuth shuffle.
// The shuffler is incremental: it shuffles the queries as they come.
// The shuffler remembers the generated order, which can be used to
// de-shuffle responses as they come.

#include "drivacy/protocol/offline/shuffle.h"

#include <algorithm>
#include <cmath>

#include "drivacy/primitives/crypto.h"
#include "drivacy/primitives/util.h"

namespace drivacy {
namespace protocol {
namespace offline {

// Constructor.
Shuffler::Shuffler(uint32_t party_id, uint32_t machine_id, uint32_t party_count,
                   uint32_t parallelism)
    : party_id_(party_id), machine_id_(machine_id), parallelism_(parallelism) {
  this->total_size_ = 0;
  // Message sizes.
  this->message_size_ =
      primitives::crypto::OnionCipherSize(party_id + 1, party_count);
  // Seed RNG.
  this->generator_ = primitives::util::SeedGenerator(party_id);
  // Mark buffers as nullptr.
  this->shuffled_messages_ = nullptr;
}

// Delete any remaining queries that were shuffled but not consumed.
Shuffler::~Shuffler() {
  if (this->shuffled_messages_ != nullptr) {
    delete[] this->shuffled_messages_;
  }
}

// Initialization for a batch.
bool Shuffler::Initialize(uint32_t machine_id, uint32_t given_size) {
  // Determine if we are initializing a fresh new batch, or
  // providing remaining sizes for a batch being initialized.
  if (this->size_.size() == this->parallelism_) {
    this->size_.clear();
    this->total_size_ = 0;
  }

  // Store size information.
  assert(this->size_.count(machine_id) == 0);
  this->total_size_ += given_size;
  this->size_.insert({machine_id, given_size});
  if (this->size_.size() < this->parallelism_) {
    return false;
  }

  // Reset indices.
  this->message_index_ = 0;

  // Store batch size.
  assert(this->total_size_ >= this->parallelism_);
  this->batch_size_ = static_cast<uint32_t>(
      std::ceil((1.0 * this->total_size_) / this->parallelism_));
  if (this->machine_id_ == this->parallelism_ &&
      this->total_size_ % this->parallelism_ > 0) {
    this->batch_size_ =
        this->total_size_ - (this->batch_size_ * (this->parallelism_ - 1));
  }

  this->shuffled_message_count_ = 0;

  // Allocate memory for shuffled messages.
  this->shuffled_messages_ =
      new unsigned char[this->batch_size_ * this->message_size_];

  // clear maps.
  this->message_machine_ids_index_ = 0;
  this->message_indices_.clear();
  for (uint32_t m = 1; m <= this->parallelism_; m++) {
    this->message_indices_[m].first = 0;
  }

  return true;
}

// Simulate a global shuffle and store relevant portion of shuffling order.
void Shuffler::PreShuffle() {
  uint32_t ceil_batch_size = static_cast<uint32_t>(
      std::ceil((1.0 * this->total_size_) / this->parallelism_));

  // Knuth shuffling.
  std::unordered_map<uint32_t, uint32_t> shuffling_order;
  std::vector<std::pair<uint32_t, uint32_t>> received(this->batch_size_);
  std::vector<std::pair<uint32_t, uint32_t>> sent(
      this->size_[this->machine_id_]);
  for (uint32_t i = 0; i < this->total_size_; i++) {
    uint32_t true_i = i;
    if (shuffling_order.count(i) == 1) {
      true_i = shuffling_order.at(i);
    }
    // Query at global index j goes to global index i after shuffling.
    uint32_t j = i;
    if (i < this->total_size_ - 1) {
      j = primitives::util::SRand32(&this->generator_, i, this->total_size_);
    }

    // Which query is really at index j, might have been swapped earlier.
    uint32_t true_j = j;
    if (shuffling_order.count(j) == 1) {
      true_j = shuffling_order.at(j);
    }

    // Who does i and j belong to?
    uint32_t receiver = i / ceil_batch_size + 1;
    uint32_t sender = 0;
    uint32_t sender_partial_sum = 0;
    while (true_j >= sender_partial_sum) {
      sender_partial_sum += this->size_.at(++sender);
    }

    // Perform swap!
    shuffling_order[j] = true_i;
    shuffling_order.erase(i);

    // Case 1: i is in this machine's bucket.
    //         this machine will receive j from its owner.
    if (receiver == this->machine_id_) {
      uint32_t local_index = i % ceil_batch_size;
      auto &[previous, target] = received[local_index];
      previous = true_j;
      target = local_index;
    }
    // Case 2: j is in this machine's bucket.
    //         this machine will send j to machine_id_i.
    if (sender == this->machine_id_) {
      uint32_t local_index =
          true_j - sender_partial_sum + this->size_.at(sender);
      auto &[target, previous] = sent[local_index];
      target = i;
      previous = local_index;
    }
    // Case 3: neither of these things is true, this->machine_id_ does not care.
  }

  // We have all the information we need in received and sent!
  shuffling_order.clear();

  // message_machine_ids_ needs to be filled in original non-shuffled order.
  this->message_machine_ids_.clear();
  this->message_machine_ids_.reserve(sent.size());
  for (auto &[target, _] : sent) {
    uint32_t target_machine_id = target / ceil_batch_size + 1;
    this->message_machine_ids_.push_back(target_machine_id);
  }

  // received[i]: the global index corresponding of the query that will end up
  //              in the ith location within this machine's bucket post shuffle.
  std::sort(received.begin(), received.end());

  // Fill in our maps!
  uint32_t owner = 0;
  uint32_t partial_sum = 0;
  for (auto &[previous, target] : received) {
    while (previous >= partial_sum) {
      partial_sum += this->size_.at(++owner);
    }
    this->message_indices_[owner].second.push_back(target);
  }
}

// Returns a vector (logically a map) from a machine_id to the count
// of messages expected to be received from that machine.
std::vector<uint32_t> Shuffler::IncomingMessagesCount() {
  std::vector<uint32_t> result(this->parallelism_ + 1, 0);
  for (uint32_t m = 1; m < this->machine_id_; m++) {
    uint32_t count = this->message_indices_.at(m).second.size();
    result[m] = count;
  }
  for (uint32_t m = this->machine_id_ + 1; m <= this->parallelism_; m++) {
    uint32_t count = this->message_indices_.at(m).second.size();
    result[m] = count;
  }
  return result;
}

// Returns a vector (logically a map) from a machine_id to the count of
// noise messages generated by this machine, but sent to that machine.
std::vector<uint32_t> Shuffler::OutgoingNoiseMessagesCount(
    uint32_t noise_size) {
  std::vector<uint32_t> result(this->parallelism_ + 1, 0);
  for (uint32_t i = 0; i < noise_size; i++) {
    uint32_t target_machine_id = this->message_machine_ids_.at(i);
    result.at(target_machine_id)++;
  }
  return result;
}

// Determines the machine the next response is meant to be sent to.
uint32_t Shuffler::MachineOfNextMessage() {
  return this->message_machine_ids_[this->message_machine_ids_index_++];
}

// Shuffle message into shuffler.
bool Shuffler::ShuffleMessage(uint32_t machine_id,
                              const types::CipherText &message) {
  // Find index.
  auto &[i, nested_map] = this->message_indices_.at(machine_id);
  uint32_t index = nested_map.at(i++);
  // Copy into index.
  unsigned char *dest = this->shuffled_messages_ + index * this->message_size_;
  memcpy(dest, message, this->message_size_);
  return ++this->shuffled_message_count_ == this->batch_size_;
}

// Get the next query in the shuffled order.
types::CipherText Shuffler::NextMessage() {
  return this->shuffled_messages_ +
         this->message_index_++ * this->message_size_;
}

// Free allocated memory for queries.
void Shuffler::FreeMessages() {
  delete[] this->shuffled_messages_;
  this->shuffled_messages_ = nullptr;
}

}  // namespace offline
}  // namespace protocol
}  // namespace drivacy
