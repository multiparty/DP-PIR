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

void Party::Listen() { this->socket_->Listen(); }

void Party::OnReceiveBatchSize(uint32_t batch_size) {
#ifdef DEBUG_MSG
  std::cout << "On Receive batch size " << party_id_ << "-" << machine_id_
            << " = " << batch_size << std::endl;
#endif
  // Sample noise.
  this->noise_ = protocol::noise::SampleNoise(
      this->party_id_, this->machine_id_, this->config_, this->table_);
  this->noise_size_ = noise_.size();
  // Update batch size and send it to next party.
  this->batch_size_ = batch_size + this->noise_size_;
  this->intra_party_socket_->BroadcastBatchSize(this->batch_size_);
  this->intra_party_socket_->ListenBatchSizes();
}

bool Party::OnReceiveBatchSize(uint32_t machine_id, uint32_t batch_size) {
#ifdef DEBUG_MSG
  std::cout << "On Receive batch size 2 " << party_id_ << "-" << machine_id_
            << " = " << machine_id << ":" << batch_size << std::endl;
#endif
  // Initialize shuffling and shuffle in the noise.
  if (this->shuffler_.Initialize(machine_id, batch_size)) {
    this->socket_->SendBatch(this->shuffler_.batch_size());
    this->shuffler_.PreShuffle();
    return true;
  }
  return false;
}

void Party::OnReceiveBatchSize2() {
#ifdef DEBUG_MSG
  std::cout << "On Receive batch size 2 (cntd) " << party_id_ << "-"
            << machine_id_ << std::endl;
#endif
  for (types::OutgoingQuery &query : this->noise_) {
    uint32_t machine_id =
        this->shuffler_.MachineOfNextQuery(query.query_state());
    this->intra_party_socket_->SendQuery(machine_id, query);
    query.Free();
  }
  this->queries_shuffled_ += this->noise_size_;
  this->noise_.clear();
}

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
  this->intra_party_socket_->SendQuery(machine_id, outgoing_query);
  outgoing_query.Free();
  if (++this->queries_shuffled_ == this->batch_size_) {
    this->queries_shuffled_ = 0;
    this->intra_party_socket_->FlushQueries();
    this->intra_party_socket_->ListenQueries(
        std::move(this->shuffler_.IncomingQueriesCount()));
  }
}

void Party::OnReceiveResponse(const types::ForwardResponse &forward) {
#ifdef DEBUG_MSG
  std::cout << "On receive response " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // Distributed two phase deshuffling - Phase 1.
  // Assign response to the machine that handled its corresponding
  // query in phase 1 of shuffling.
  uint32_t machine_id = this->shuffler_.MachineOfNextResponse();
  this->intra_party_socket_->SendResponse(machine_id, forward);
  if (++this->responses_deshuffled_ == this->batch_size_) {
    this->responses_deshuffled_ = 0;
    this->intra_party_socket_->FlushResponses();
    this->intra_party_socket_->ListenResponses();
  }
}

void Party::OnReceiveQuery(uint32_t machine_id,
                           const types::ForwardQuery &query) {
#ifdef DEBUG_MSG
  std::cout << "On receive query 2 " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  // Distributed two phase shuffling - Phase 2.
  // Shuffle query locally, and wait until all queries are shuffled.
  if (this->shuffler_.ShuffleQuery(machine_id, query)) {
    this->intra_party_socket_->BroadcastQueriesReady();
  }
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
  if (this->shuffler_.DeshuffleResponse(machine_id, outgoing_response)) {
    this->intra_party_socket_->BroadcastResponsesReady();
  }
}

void Party::OnQueriesReady(uint32_t machine_id) {
#ifdef DEBUG_MSG
  std::cout << "On queries ready! " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  if (++this->query_machines_ready_ == this->config_.parallelism()) {
    this->query_machines_ready_ = 0;
    this->SendQueries();
  }
}
void Party::OnResponsesReady(uint32_t machine_id) {
#ifdef DEBUG_MSG
  std::cout << "On response ready! " << party_id_ << "-" << machine_id_
            << std::endl;
#endif
  if (++this->response_machines_ready_ == this->config_.parallelism()) {
    this->response_machines_ready_ = 0;
    this->SendResponses();
  }
}

void Party::SendQueries() {
#ifdef DEBUG_MSG
  std::cout << "send queries " << party_id_ << "-" << machine_id_ << std::endl;
#endif
  for (uint32_t i = 0; i < this->shuffler_.batch_size(); i++) {
    types::ForwardQuery query = this->shuffler_.NextQuery();
    this->socket_->SendQuery(query);
    delete[] query;  // memory is allocated inside shuffler_
  }
  this->socket_->FlushQueries();
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
  for (uint32_t i = this->noise_size_; i < this->batch_size_; i++) {
    types::Response &response = this->shuffler_.NextResponse();
    this->socket_->SendResponse(response);
  }
  this->socket_->FlushResponses();
}

}  // namespace parties
}  // namespace drivacy
