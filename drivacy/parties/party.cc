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

#include "drivacy/protocol/query.h"
#include "drivacy/protocol/response.h"

namespace drivacy {
namespace parties {

void Party::Listen() { this->socket_->Listen(); }

void Party::OnReceiveBatch(uint32_t batch_size) {
  this->batch_size_ = batch_size;
  this->socket_->SendBatch(this->batch_size_);
  this->shuffler_.Initialize(this->batch_size_);
}

void Party::OnReceiveQuery(const types::IncomingQuery &query) {
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
  // Distributed two phase shuffling - Phase 2.
  // Shuffle query locally, and wait until all queries are shuffled.
  if (this->shuffler_.ShuffleQuery(machine_id, query)) {
    this->intra_party_socket_->BroadcastQueriesReady();
  }
}

void Party::OnReceiveResponse(uint32_t machine_id,
                              const types::Response &response) {
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
  if (++this->query_machines_ready_ == this->config_.parallelism()) {
    this->query_machines_ready_ = 0;
    this->SendQueries();
  }
}
void Party::OnResponsesReady(uint32_t machine_id) {
  if (++this->response_machines_ready_ == this->config_.parallelism()) {
    this->response_machines_ready_ = 0;
    this->SendResponses();
  }
}

void Party::SendQueries() {
  for (uint32_t i = 0; i < this->batch_size_; i++) {
    types::ForwardQuery &query = this->shuffler_.NextQuery();
    this->socket_->SendQuery(query);
    delete[] query;  // memory is allocated inside shuffler_
  }
  this->socket_->FlushQueries();
}

void Party::SendResponses() {
  for (uint32_t i = 0; i < this->batch_size_; i++) {
    types::Response &response = this->shuffler_.NextResponse();
    this->socket_->SendResponse(response);
  }
  this->socket_->FlushResponses();
}

}  // namespace parties
}  // namespace drivacy
