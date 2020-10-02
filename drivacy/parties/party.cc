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

#include "drivacy/protocol/backend.h"
#include "drivacy/protocol/query.h"
#include "drivacy/protocol/response.h"

namespace drivacy {
namespace parties {

void Party::Listen() { this->socket_->Listen(); }

void Party::OnReceiveQuery(const types::IncomingQuery &query) {
  if (this->party_id_ < this->config_.parties()) {
    // Process query.
    types::OutgoingQuery outgoing_query =
        protocol::query::ProcessQuery(this->party_id_, query, this->config_);
    // Store the query state.
    query_state_.preshare = outgoing_query.preshare();
    // Send the query over socket.
    this->socket_->SendQuery(outgoing_query);
  } else {
    // Process query creating a response, send it over socket.
    types::Response response =
        protocol::backend::QueryToResponse(query, config_, table_);
    this->socket_->SendResponse(response);
  }
}

void Party::OnReceiveResponse(const types::Response &response) {
  uint64_t preshare = this->query_state_.preshare;
  types::Response outgoing_response =
      protocol::response::ProcessResponse(response, preshare);
  this->socket_->SendResponse(outgoing_response);
}

}  // namespace parties
}  // namespace drivacy
