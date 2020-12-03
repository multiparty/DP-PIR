// Copyright 2020 multiparty.org

// This file defines the "Party" class, which represents
// A party/machine-specific instance of our protocol.
//
// When deployed, every machine's code will construct exactly a single
// instance of this class, and use it as the main control flow for the protocol.
//
// In a simulated environemnt, when all parties are run locally. The process
// will construct several instances of this "Party" class, one per logical
// party.

#include "drivacy/parties/offline/party.h"

#include <memory>
#include <utility>

#include "drivacy/primitives/crypto.h"

#define ELAPSED(i)                                                         \
  std::chrono::duration_cast<std::chrono::milliseconds>(end##i - start##i) \
      .count()

#define TIMER(i) auto start##i = std::chrono::system_clock::now()

#define TIME(tag, i)                              \
  auto end##i = std::chrono::system_clock::now(); \
  std::cout << tag << " " << ELAPSED(i) << std::endl

namespace drivacy {
namespace parties {
namespace offline {

// Begin by reading the batch size from the previous party (blocking).
void Party::Start() {
#ifdef DEBUG_MSG
  std::cout << "Starting ... " << party_id_ << "-" << machine_id_ << std::endl;
#endif
  this->inter_party_socket_.ReadBatchSize();
  // Expect the machines to tell us their batch size.
  this->intra_party_socket_.CollectBatchSizes();
  // First, inject the noise queries in.
  this->InjectNoise();
  // Now we can listen to incoming queries (from previous party or from
  // machines parallel).
  this->first_query_ = true;
  this->listener_.ListenToMessages();
  // After all queries are handled, broadcast ready.
  this->intra_party_socket_.BroadcastMessagesReady();
  this->intra_party_socket_.CollectMessagesReady();
  // All queries are ready, we can move to the next party now!
  this->SendMessages();
  // Save the common reference we got.
  this->SaveCommonReference();
}

// Batch size information and handling.
void Party::OnReceiveBatchSize(uint32_t batch_size) {
#ifdef DEBUG_MSG
  std::cout << "On Receive batch size " << party_id_ << "-" << machine_id_
            << " = " << batch_size << std::endl;
#endif
  // Sample noise.
  TIMER(0);
  this->noise_size_ = 0;
  this->noise_ = protocol::offline::noise::SampleNoiseHistogram(
      this->machine_id_, this->config_.parallelism(), this->table_.size(),
      this->span_, this->cutoff_);
  for (uint32_t count : this->noise_) {
    this->noise_size_ += count;
  }
  TIME("Sampled noise", 0);
  // Update batch size.
  this->input_batch_size_ = batch_size + this->noise_size_;
  // Send batch size to all other machines of our same party.
  this->intra_party_socket_.BroadcastBatchSize(this->input_batch_size_);
}

bool Party::OnReceiveBatchSize(uint32_t machine_id, uint32_t batch_size) {
#ifdef DEBUG_MSG
  std::cout << "On Receive batch size 2 " << party_id_ << "-" << machine_id_
            << " = " << machine_id << ":" << batch_size << std::endl;
#endif
  // Initialize shuffler, shuffler will let us know if it has all the batch
  // size information it needs.
  return this->shuffler_.Initialize(machine_id, batch_size);
}

void Party::OnCollectedBatchSizes() {
#ifdef DEBUG_MSG
  std::cout << "On Collected batch size 2 " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // We have all the batch size information.
  // We can compute our output batch size and send it to the next party.
  this->output_batch_size_ = this->shuffler_.batch_size();
  this->inter_party_socket_.SendBatchSize(this->output_batch_size_);
  // Simulate a global preshuffle to use later for shuffling incrementally.
  TIMER(0);
  this->shuffler_.PreShuffle();
  TIME("PreShuffled", 0);
  // Give the socket the number of queries (and noise queries) to exchange.
  this->intra_party_socket_.SetMessageCounts(
      std::move(this->shuffler_.IncomingMessagesCount()));
  this->intra_party_socket_.BroadcastNoiseQueryCounts(
      std::move(this->shuffler_.OutgoingNoiseMessagesCount(this->noise_size_)));
}

void Party::InjectNoise() {
#ifdef DEBUG_MSG
  std::cout << "Injecting Noise " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // Make the noise queries.
  uint32_t seed = this->config_.parallelism() *
                  (this->input_batch_size_ - this->noise_size_);
  seed += (this->machine_id_ - 1) * this->noise_size_;
  std::vector<std::vector<types::Message>> noise =
      protocol::offline::noise::CommonReferenceForNoise(
          this->party_id_, this->config_.parties(), this->noise_size_, seed);

  // Shuffle in the noise queries.
  TIMER(0);
  for (std::vector<types::Message> &vec : noise) {
    std::unique_ptr<unsigned char[]> cipher = primitives::crypto::OnionEncrypt(
        vec, this->config_, this->party_id_ + 1);
    uint32_t machine_id = this->shuffler_.MachineOfNextMessage();
    this->intra_party_socket_.SendMessage(machine_id, cipher.get());
    // In case we are receiving noise from other parallel machines
    // while operating on these noise!
    this->listener_.ListenToNoiseMessagesNonblocking();
  }
  TIME("Injected noise", 0);

  // Finish up any remaining noise queries sent to this party by other parties.
  this->listener_.ListenToNoiseMessages();
}

// Message handling.
void Party::OnReceiveMessage(const types::CipherText &message) {
#ifdef DEBUG_MSG
  std::cout << "On receive message " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  if (this->first_query_) {
    this->first_query_ = false;
    this->OnStart();
  }

  // Process message.
  types::OnionMessage onion_message =
      primitives::crypto::SingleLayerOnionDecrypt(this->party_id_, message,
                                                  this->config_);

  // Add the common reference to the map.
  assert(this->common_references_.count(onion_message.tag()) == 0);
  this->common_references_.insert(
      {onion_message.tag(), onion_message.common_reference()});

  // Distributed two phase shuffling - Phase 1.
  // Assign query to some machine.
  uint32_t machine_id = this->shuffler_.MachineOfNextMessage();
  this->intra_party_socket_.SendMessage(machine_id, onion_message.cipher());
}

void Party::OnReceiveMessage(uint32_t machine_id,
                             const types::CipherText &message) {
#ifdef DEBUG_MSG
  std::cout << "On receive message 2 " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  if (this->first_query_) {
    this->first_query_ = false;
    this->OnStart();
  }

  // Distributed two phase shuffling - Phase 2.
  // Shuffle query locally, and wait until all queries are shuffled.
  this->shuffler_.ShuffleMessage(machine_id, message);
}

void Party::SendMessages() {
#ifdef DEBUG_MSG
  std::cout << "send messages " << party_id_ << "-" << machine_id_ << std::endl;
#endif
  for (uint32_t i = 0; i < this->output_batch_size_; i++) {
    types::CipherText cipher = this->shuffler_.NextMessage();
    this->inter_party_socket_.SendMessage(cipher);
  }
  this->shuffler_.FreeMessages();
}

void Party::SaveCommonReference() {
#ifdef DEBUG_MSG
  std::cout << "save common references " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // We are done, wait for next party to tell us they are done before timing.
  this->inter_party_socket_.WaitForDone();
  this->OnEnd();
  this->inter_party_socket_.SendDone();

  /*
  for (const auto &[tag, common_reference] : this->common_references_) {
    std::cout << tag << std::endl;
    std::cout << common_reference.next_tag << " "
              << common_reference.incremental_share.x << " "
              << common_reference.incremental_share.y << " "
              << common_reference.preshare << std::endl;
  }
  */
}

// Timing functions.
void Party::OnStart() { this->start_time_ = std::chrono::system_clock::now(); }
void Party::OnEnd() {
  auto end = std::chrono::system_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - this->start_time_);
  std::cout << "Total time: " << diff.count() << std::endl;
}

}  // namespace offline
}  // namespace parties
}  // namespace drivacy
