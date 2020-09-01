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
#include <string>
#include <unordered_map>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/messages.pb.h"
#include "uWebSockets/App.h"

namespace drivacy {
namespace io {
namespace socket {

class ClientSocket : public AbstractSocket {
 public:
  ClientSocket(uint32_t _, QueryListener query_listener, ResponseListener __)
      : query_listener_(query_listener), client_counter_(0) {}

  // We can never send queries to clients!
  void SendQuery(uint32_t client, const types::Query &query) const override {
    assert(false);
  };

  // We can send responses to clients!
  void SendResponse(uint32_t client_id,
                    const types::Response &response) const override;

  void Listen() override;

 private:
  QueryListener query_listener_;

  // Used to generate unique ids for new clients.
  uint32_t client_counter_;

  // Client id to its socket!
  std::unordered_map<uint32_t, uWS::WebSocket<false, true> *> sockets_;

  // Handle incoming query from a client (parses and then calls QueryListener).
  void HandleQuery(std::string message, uint32_t client_id) const;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_CLIENT_SOCKET_H_
