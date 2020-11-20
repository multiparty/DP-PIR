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

#include "drivacy/protocol/query.h"
#include "drivacy/protocol/response.h"

namespace drivacy {
namespace parties {

void Party::Listen() { this->socket_->Listen(); }

void Party::OnReceiveBatch(uint32_t batch_size) {
  this->batch_size_ = batch_size;
  this->shuffler_.Initialize(this->batch_size_);
}

void Party::OnReceiveQuery(const types::IncomingQuery &query) {
  // Process query.
  types::OutgoingQuery outgoing_query =
      protocol::query::ProcessQuery(this->party_id_, query, this->config_);

  // Shuffle and store query state.
  if (this->shuffler_.ShuffleQuery(outgoing_query)) {
    this->SendQueries();
  }
}

void Party::OnReceiveResponse(const types::ForwardResponse &forward) {
  types::Response response = types::Response::Deserialize(forward);
  // Process response.
  types::QueryState query_state = this->shuffler_.NextQueryState();
  types::Response outgoing_response =
      protocol::response::ProcessResponse(response, query_state);

  // Deshuffle response.
  if (this->shuffler_.DeshuffleResponse(outgoing_response)) {
    this->SendResponses();
  }
}

void Party::SendQueries() {
  this->socket_->SendBatch(this->batch_size_);
  for (uint32_t i = 0; i < this->batch_size_; i++) {
    types::OutgoingQuery query = this->shuffler_.NextQuery();
    this->socket_->SendQuery(query.Serialize());
    query.Free();
  }
  this->socket_->FlushQueries();
}

void Party::SendResponses() {
  for (uint32_t i = 0; i < this->batch_size_; i++) {
    types::Response response = this->shuffler_.NextResponse();
    this->socket_->SendResponse(response);
    response.Free();
  }
  this->socket_->FlushResponses();
}

}  // namespace parties
}  // namespace drivacy
