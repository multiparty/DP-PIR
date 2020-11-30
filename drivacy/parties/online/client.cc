// Copyright 2020 multiparty.org

// This file defines the "Client" class, which represents
// a single client in our protocol. The client is responsible for creating
// queries and handling responses.
//
// When deployed, this client communicates with the appropriate (server) parties
// on the wire. In a simulated enviornment, the client and parties are run
// locally in the same process, and they interact directly via the enclosing
// simulation code.

#include "drivacy/parties/online/client.h"

#include "drivacy/protocol/online/client.h"
#include "drivacy/util/fake.h"

namespace drivacy {
namespace parties {
namespace online {

void Client::Listen() { this->socket_.Listen(); }

void Client::MakeQuery(uint64_t value) {
  // Create query via client protocol.
  types::Query query = protocol::online::client::CreateQuery(
      value, fake::FakeIt(this->config_.parties()));
  // this->commons_list_.at(this->common_index_++));
  // Store state to use for when reconstructing corresponding response.
  this->state_.queries.push_back(value);
  this->state_.preshares.push_back(0);  // TODO(babman): need one extra share.
  // Send via socket.
  this->socket_.SendQuery(query);
}

void Client::OnReceiveResponse(const types::Response &response) {
  // Retrieve stored state for this response.
  uint64_t query = this->state_.queries.front();
  types::QueryState preshare = this->state_.preshares.front();
  // Remove the stored state belonging to this response.
  this->state_.queries.pop_front();
  this->state_.preshares.pop_front();
  // Reconstruct response using client protocol.
  uint64_t response_value =
      protocol::online::client::ReconstructResponse(response, preshare);
  this->response_handler_(query, response_value);
}

}  // namespace online
}  // namespace parties
}  // namespace drivacy
