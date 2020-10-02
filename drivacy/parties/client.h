// Copyright 2020 multiparty.org

// This file defines the "Client" class, which represents
// a single client in our protocol. The client is responsible for creating
// queries and handling responses.
//
// When deployed, this client communicates with the appropriate (server) parties
// on the wire. In a simulated enviornment, the client and parties are run
// locally in the same process, and they interact directly via the enclosing
// simulation code.

#ifndef DRIVACY_PARTIES_CLIENT_H_
#define DRIVACY_PARTIES_CLIENT_H_

#include <cstdint>
#include <memory>

#include "absl/functional/bind_front.h"
#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {

// A function taking two arguments: the original query and its response values.
using ResponseHandler = std::function<void(uint64_t, uint64_t)>;

class Client {
 public:
  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  explicit Client(const types::Configuration &config,
                  io::socket::SocketFactory socket_factory);

  // Not movable or copyable: when an instance is constructed, a pointer to it
  // is stored in the socket (e.g. look at io/simulated_socket.[cc|h]).
  // If the object is later moved or copied, the pointer might become invalid.
  Client(Client &&other) = delete;
  Client &operator=(Client &&other) = delete;
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  // Set the response handler.
  void SetOnResponseHandler(ResponseHandler response_handler) {
    this->response_handler_ = response_handler;
  }

  // Make and send a query using our protocol targeting key = value.
  void MakeQuery(uint64_t value);

 private:
  const types::Configuration &config_;
  std::unique_ptr<io::socket::AbstractSocket> socket_;
  types::ClientState state_;
  ResponseHandler response_handler_;

  // Executed when a response is received, ends up calling response handler.
  void OnReceiveResponse(const types::Response &response);
};

}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_CLIENT_H_
