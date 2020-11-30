// Copyright 2020 multiparty.org

// This file defines the "Client" class, which represents
// a single client in our protocol. The client is responsible for creating
// queries and handling responses.
//
// When deployed, this client communicates with the appropriate (server) parties
// on the wire. In a simulated enviornment, the client and parties are run
// locally in the same process, and they interact directly via the enclosing
// simulation code.

#ifndef DRIVACY_PARTIES_OFFLINE_CLIENT_H_
#define DRIVACY_PARTIES_OFFLINE_CLIENT_H_

#include <cassert>
#include <cstdint>
#include <vector>

#include "drivacy/io/websocket_client.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {
namespace offline {

class Client : public io::socket::WebSocketClientListener {
 public:
  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  Client(uint32_t machine_id, const types::Configuration &config)
      : machine_id_(machine_id),
        config_(config),
        socket_(machine_id, config, this) {}

  // Not movable or copyable: when an instance is constructed, a pointer to it
  // is stored in the socket (e.g. look at io/simulated_socket.[cc|h]).
  // If the object is later moved or copied, the pointer might become invalid.
  Client(Client &&other) = delete;
  Client &operator=(Client &&other) = delete;
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  // Subscribe to the service by making count many preprocessing requests.
  void Subscribe(uint32_t count);

  // Useless...
  void OnReceiveResponse(const types::Response &response) { assert(false); }

 private:
  // Configuration.
  uint32_t machine_id_;
  const types::Configuration &config_;
  // Websocket client.
  io::socket::WebSocketClient socket_;
  // State for response handling.
  std::vector<std::vector<types::Message>> common_references_;
  // Saving the common references.
  void SaveCommonReference();
};

}  // namespace offline
}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_OFFLINE_CLIENT_H_
