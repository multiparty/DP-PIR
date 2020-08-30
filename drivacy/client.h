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
#include "drivacy/types/messages.pb.h"
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
  Client(const types::Configuration &config) : config_(config) {
    this->socket_ = std::make_unique<S>(
        0, absl::bind_front(&Client<S>::Unused, this),
        absl::bind_front(&Client<S>::OnReceiveResponse, this), config);
  }

  uint32_t party_id() const { return this->party_id_; }
  void SetOnResponseHandler(ResponseHandler response_handler) {
    this->response_handler_ = response_handler;
  }

  void MakeQuery(uint64_t value) {
    types::Query query =
        protocol::client::CreateQuery(value, this->config_, &this->state_);
    this->socket_->SendQuery(1, query);
  }

 private:
  void Unused(uint32_t party, const types::Query &query) { assert(false); }
  void OnReceiveResponse(uint32_t party, const types::Response &response) {
    const auto &[q, r] =
        protocol::client::ReconstructResponse(response, &this->state_);
    this->response_handler_(q, r);
  }

  const types::Configuration &config_;
  std::unique_ptr<S> socket_;
  types::ClientState state_;
  ResponseHandler response_handler_;
};

}  // namespace drivacy

#endif  // DRIVACY_CLIENT_H_
