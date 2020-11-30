// Copyright 2020 multiparty.org

// This file defines the "Client" class, which represents
// a single client in our protocol. The client is responsible for creating
// queries and handling responses.
//
// When deployed, this client communicates with the appropriate (server) parties
// on the wire. In a simulated enviornment, the client and parties are run
// locally in the same process, and they interact directly via the enclosing
// simulation code.

#ifndef DRIVACY_PARTIES_ONLINE_CLIENT_H_
#define DRIVACY_PARTIES_ONLINE_CLIENT_H_

#include <cstdint>
#include <functional>

#include "drivacy/io/websocket_client.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {
namespace online {

// A function taking two arguments: the original query and its response values.
using ResponseHandler = std::function<void(uint64_t, uint64_t)>;

class Client : public io::socket::WebSocketClientListener {
 public:
  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  Client(uint32_t machine_id, const types::Configuration &config)
      : machine_id_(machine_id),
        config_(config),
        socket_(machine_id, config, this),
        common_index_(0) {}

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

  // Called to start the listening on the socket (blocking!)
  void Listen();

  // Make and send a query using our protocol targeting key = value.
  void MakeQuery(uint64_t value);

  // Executed when a batch size, query, or response are received.
  void OnReceiveResponse(const types::Response &response) override;

 private:
  // Configuration.
  uint32_t machine_id_;
  const types::Configuration &config_;
  // Websocket client.
  io::socket::WebSocketClient socket_;
  // State for response handling.
  types::ClientState state_;
  ResponseHandler response_handler_;
  // Common reference lists and maps produced by offline stage.
  uint32_t common_index_;
  std::vector<std::vector<types::Message>> commons_list_;
};

}  // namespace online
}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_ONLINE_CLIENT_H_
