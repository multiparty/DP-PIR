// Copyright 2020 multiparty.org

// This file defines the "Client" class, which represents
// a single client in our protocol. The client is responsible for creating
// queries and handling responses.
//
// When deployed, this client communicates with the appropriate (server) parties
// on the wire. In a simulated enviornment, the client and parties are run
// locally in the same process, and they interact directly via the enclosing
// simulation code.

#ifndef DRIVACY_CLIENT_H_
#define DRIVACY_CLIENT_H_

#include <cstdint>
#include <memory>
#include <type_traits>

#include "absl/functional/bind_front.h"
#include "drivacy/io/abstract_socket.h"
#include "drivacy/protocol/client.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {

// A function taking two arguments: the original query and its response values.
using ResponseHandler = std::function<void(uint64_t, uint64_t)>;

template <typename S>
class Client {
  // Ensure S inherits AbstractSocket.
  static_assert(std::is_base_of<drivacy::io::socket::AbstractSocket, S>::value,
                "S must inherit from AbstractSocket");

 public:
  // Not movable or copyable: when an instance is constructed, a pointer to it
  // is stored in the socket (e.g. look at io/simulated_socket.[cc|h]).
  // If the object is later moved or copied, the pointer might become invalid.
  Client(Client &&other) = delete;
  Client &operator=(Client &&other) = delete;
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  explicit Client(const types::Configuration &config) : config_(config) {
    this->socket_ = std::make_unique<S>(
        0, absl::bind_front(&Client<S>::Unused, this),
        absl::bind_front(&Client<S>::OnReceiveResponse, this), config);
  }

  void SetOnResponseHandler(ResponseHandler response_handler) {
    this->response_handler_ = response_handler;
  }

  void MakeQuery(uint64_t value) {
    // Create query via client protocol.
    types::OutgoingQuery query =
        protocol::client::CreateQuery(value, this->config_);
    // Store state to use for when reconstructing corresponding response.
    this->state_.queries.push_back(value);
    this->state_.preshares.push_back(query.preshare());
    // Send via socket.
    this->socket_->SendQuery(query);
  }

 private:
  void Unused(const types::IncomingQuery &_) { assert(false); }
  void OnReceiveResponse(const types::Response &response) {
    // Retrieve stored state for this response.
    uint64_t query = this->state_.queries.front();
    uint64_t preshare = this->state_.preshares.front();
    // Remove the stored state belonging to this response.
    this->state_.queries.pop_front();
    this->state_.preshares.pop_front();
    // Reconstruct response using client protocol.
    uint64_t response_value =
        protocol::client::ReconstructResponse(response, preshare);
    this->response_handler_(query, response_value);
  }

  const types::Configuration &config_;
  std::unique_ptr<S> socket_;
  types::ClientState state_;
  ResponseHandler response_handler_;
};

}  // namespace drivacy

#endif  // DRIVACY_CLIENT_H_
