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
  uint64_t preshare = this->query_state_.preshare;
  types::Response outgoing_response =
      protocol::response::ProcessResponse(response, preshare);
  this->client_socket_->SendResponse(outgoing_response);
}

}  // namespace parties
}  // namespace drivacy
