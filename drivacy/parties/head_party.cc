// Copyright 2020 multiparty.org

// This file defines the "HeadParty" class. A specialized Party that interfaces
// with clients.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#include "drivacy/parties/head_party.h"

#include "drivacy/protocol/response.h"

namespace drivacy {
namespace parties {

void HeadParty::Listen() {
  Party::Listen();
  this->client_socket_->Listen();
}

void HeadParty::OnReceiveResponse(const types::Response &response) {
  // Process response.
  types::QueryState query_state = this->shuffler_.NextQueryState();
  types::Response outgoing_response =
      protocol::response::ProcessResponse(response, query_state);

  // Deshuffle response.
  bool done = this->shuffler_.DeshuffleResponse(outgoing_response);
  if (!done) return;

  // Send the responses over socket.
  for (uint32_t i = 0; i < this->size_; i++) {
    outgoing_response = this->shuffler_.NextResponse();
    this->client_socket_->SendResponse(outgoing_response);
  }
}

}  // namespace parties
}  // namespace drivacy
