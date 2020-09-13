// Copyright 2020 multiparty.org

// The real server-side web-socket interface for communicating with client.
// The client side socket is in javascript under //client/.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#ifndef DRIVACY_IO_CLIENT_SOCKET_H_
#define DRIVACY_IO_CLIENT_SOCKET_H_

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "uWebSockets/App.h"

namespace drivacy {
namespace io {
namespace socket {

class ClientSocket : public AbstractSocket {
 public:
  ClientSocket(uint32_t party_id, QueryListener query_listener,
               ResponseListener _, const types::Configuration &config)
      : AbstractSocket(party_id, query_listener, _, config) {}

  // We can never send queries to clients!
  void SendQuery(const types::OutgoingQuery &query) override { assert(false); }

  // We can send responses to clients!
  void SendResponse(const types::Response &response) override;

  // We need to explicitly listen to incoming connections: blocking.
  void Listen() override;

 private:
  // Client sockets in order of receiving queries (not connecting).
  std::list<uWS::WebSocket<false, true> *> sockets_;

  // Handle incoming query from a client (parses and then calls QueryListener).
  void HandleQuery(const std::string &msg) const;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_CLIENT_SOCKET_H_
