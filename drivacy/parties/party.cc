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

#include "drivacy/parties/party.h"

#include <chrono>
#include <utility>

#include "drivacy/protocol/noise.h"
#include "drivacy/protocol/query.h"
#include "drivacy/protocol/response.h"

#define ELAPSED(i)                                                         \
  std::chrono::duration_cast<std::chrono::milliseconds>(end##i - start##i) \
      .count()

#define TIMER(i) auto start##i = std::chrono::system_clock::now()

#define TIME(tag, i)                              \
  auto end##i = std::chrono::system_clock::now(); \
  std::cout << tag << " " << ELAPSED(i) << std::endl

namespace drivacy {
namespace parties {

// Begin by reading the batch size from the previous party (blocking).
void Party::Start() {
#ifdef DEBUG_MSG
  std::cout << "Starting ... " << party_id_ << "-" << machine_id_ << std::endl;
#endif
  // Our protocol's communication flow explicitly encoded.
  while (this->batches_ == 0 || this->batch_counter_++ < this->batches_) {
    this->inter_party_socket_.ReadBatchSize();
    // Expect the machines to tell us their batch size.
    this->intra_party_socket_.CollectBatchSizes();
    // First, inject the noise queries in.
    this->InjectNoise();
    // Now we can listen to incoming queries (from previous party or from
    // machines parallel).
    this->listener_.ListenToQueries();
    // After all queries are handled, broadcast ready.
    this->intra_party_socket_.BroadcastQueriesReady();
    this->intra_party_socket_.CollectQueriesReady();
    // All queries are ready, we can move to the next party now!
    this->SendQueries();
    // Now we listen to incoming responses (from previous party or from parallel
    // machines).
    this->listener_.ListenToResponses();
    // After all responses are handled, broadcast ready.
    this->intra_party_socket_.BroadcastResponsesReady();
    this->intra_party_socket_.CollectResponsesReady();
    // All responses are ready, we can forward them to the previous party!
    this->SendResponses();
  }
}

// Batch size information and handling.
void Party::OnReceiveBatchSize(uint32_t batch_size) {
#ifdef DEBUG_MSG
  std::cout << "On Receive batch size " << party_id_ << "-" << machine_id_
            << " = " << batch_size << std::endl;
#endif
  // Sample noise.
  TIMER(0);
  auto noise_pair = protocol::noise::SampleNoise(
      this->machine_id_, this->config_.parallelism(), this->table_.size(),
      this->span_, this->cutoff_);
  TIME("Sampled noise", 0);
  this->noise_size_ = noise_pair.first;
  this->noise_counts_ = noise_pair.second;
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
  this->intra_party_socket_.SetQueryCounts(
      std::move(this->shuffler_.IncomingQueriesCount()));
  this->intra_party_socket_.BroadcastNoiseQueryCounts(
      std::move(this->shuffler_.OutgoingNoiseQueriesCount(this->noise_size_)));
}

void Party::InjectNoise() {
#ifdef DEBUG_MSG
  std::cout << "Injecting Noise " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  if (this->noise_size_ == 0) {
    return;
  }

  // Make the noise queries.
  TIMER(0);
  this->noise_ = protocol::noise::MakeNoisyQueries(
      this->party_id_, this->machine_id_, this->config_, this->table_,
      this->noise_counts_);
  TIME("Made noise queries", 0);

  // Shuffle in the noise queries.
  TIMER(1);
  for (types::OutgoingQuery &query : this->noise_) {
    uint32_t machine_id =
        this->shuffler_.MachineOfNextQuery(query.query_state());
    this->intra_party_socket_.SendQuery(machine_id, query);
    query.Free();
    // In case we are receiving noise from other parallel machines
    // while operating on these noise!
    this->listener_.ListenToNoiseQueriesNonblocking();
  }
  TIME("Injected noise", 1);

  // Clear noise, only keeping its size.
  this->noise_counts_.clear();
  this->noise_.clear();

  // Finish up any remaining noise queries sent to this party by other parties.
  this->listener_.ListenToNoiseQueries();
}

// Query handling.
void Party::OnReceiveQuery(const types::IncomingQuery &query) {
#ifdef DEBUG_MSG
  std::cout << "On receive query " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // Process query.
  types::OutgoingQuery outgoing_query =
      protocol::query::ProcessQuery(this->party_id_, query, this->config_);

  // Distributed two phase shuffling - Phase 1.
  // Assign query to some machine.
  uint32_t machine_id =
      this->shuffler_.MachineOfNextQuery(outgoing_query.query_state());
  this->intra_party_socket_.SendQuery(machine_id, outgoing_query);
  outgoing_query.Free();
}

void Party::OnReceiveQuery(uint32_t machine_id,
                           const types::ForwardQuery &query) {
#ifdef DEBUG_MSG
  std::cout << "On receive query 2 " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // Distributed two phase shuffling - Phase 2.
  // Shuffle query locally, and wait until all queries are shuffled.
  this->shuffler_.ShuffleQuery(machine_id, query);
}

void Party::SendQueries() {
#ifdef DEBUG_MSG
  std::cout << "send queries " << party_id_ << "-" << machine_id_ << std::endl;
#endif
  for (uint32_t i = 0; i < this->output_batch_size_; i++) {
    types::ForwardQuery query = this->shuffler_.NextQuery();
    this->inter_party_socket_.SendQuery(query);
  }
  this->shuffler_.FreeQueries();
}

// Response handling.
void Party::OnReceiveResponse(const types::ForwardResponse &forward) {
#ifdef DEBUG_MSG
  std::cout << "On receive response " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // Distributed two phase deshuffling - Phase 1.
  // Assign response to the machine that handled its corresponding
  // query in phase 1 of shuffling.
  uint32_t machine_id = this->shuffler_.MachineOfNextResponse();
  this->intra_party_socket_.SendResponse(machine_id, forward);
}

void Party::OnReceiveResponse(uint32_t machine_id,
                              const types::Response &response) {
#ifdef DEBUG_MSG
  std::cout << "On receive response 2 " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // Process response.
  types::QueryState &query_state = this->shuffler_.NextQueryState(machine_id);
  types::Response outgoing_response =
      protocol::response::ProcessResponse(response, query_state);

  // Distributed two phase deshuffling - Phase 2.
  // Deshuffle response locally, and wait until all responses are deshuffled.
  this->shuffler_.DeshuffleResponse(machine_id, outgoing_response);
}

void Party::SendResponses() {
#ifdef DEBUG_MSG
  std::cout << "send responses " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // Ignore noise added in by this party.
  for (uint32_t i = 0; i < this->noise_size_; i++) {
    this->shuffler_.NextResponse();
  }
  // Handle the non-noise responses.
  for (uint32_t i = this->noise_size_; i < this->input_batch_size_; i++) {
    types::Response &response = this->shuffler_.NextResponse();
    this->inter_party_socket_.SendResponse(response);
  }
}

}  // namespace parties
}  // namespace drivacy
