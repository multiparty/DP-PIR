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

#include <utility>

#include "drivacy/protocol/noise.h"
#include "drivacy/protocol/query.h"
#include "drivacy/protocol/response.h"

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
  this->noise_ = protocol::noise::SampleNoise(
      this->party_id_, this->machine_id_, this->config_, this->table_,
      this->span_, this->cutoff_);
  this->noise_size_ = noise_.size();
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
  this->shuffler_.PreShuffle();
  this->intra_party_socket_.SetQueryCounts(
      std::move(this->shuffler_.IncomingQueriesCount()));
  // Shuffle in the noise queries.
  for (types::OutgoingQuery &query : this->noise_) {
    uint32_t machine_id =
        this->shuffler_.MachineOfNextQuery(query.query_state());
    this->intra_party_socket_.SendQuery(machine_id, query);
    query.Free();
  }
  this->noise_.clear();
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
