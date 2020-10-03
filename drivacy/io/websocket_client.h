// Copyright 2020 multiparty.org

// The real client-side web-socket interface.
//
// This is only used when running an independent (not a local simulation)
// c++ client, which will use this file to communicate with the
// first frontend server via websockets.

#ifndef DRIVACY_IO_WEBSOCKET_CLIENT_H_
#define DRIVACY_IO_WEBSOCKET_CLIENT_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "third_party/easywsclient/easywsclient.hpp"

namespace drivacy {
namespace io {
namespace socket {

class WebSocketClient : public AbstractSocket {
 public:
  WebSocketClient(uint32_t party_id, QueryListener _,
                  ResponseListener response_listener,
                  const types::Configuration &config);

  // Factory function used to simplify construction of inheriting sockets.
  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, QueryListener query_listener,
      ResponseListener response_listener, const types::Configuration &config) {
    return std::make_unique<WebSocketClient>(party_id, query_listener,
                                             response_listener, config);
  }

  // This is how we send queries to the frontend party!
  void SendQuery(const types::OutgoingQuery &query) override;

  // We can never send responses as a client.
  void SendResponse(const types::Response &response) override { assert(false); }

  // Start the socket and listen to messages: blocking.
  void Listen() override;

  // Flush is useless, this socket flushes at every send...
  void FlushQueries() override { assert(false); }
  void FlushResponses() override { assert(false); }

 private:
  // The client socket.
  easywsclient::WebSocket *socket_;

  // Handle incoming responses from server (calls response_listener_).
  void HandleResponse(const std::vector<uint8_t> &msg) const;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_WEBSOCKET_CLIENT_H_
