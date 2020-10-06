// Copyright 2020 multiparty.org

// The real server-side web-socket interface for communicating with client.
// The client side socket is in javascript under //client/.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#ifndef DRIVACY_IO_WEBSOCKET_SERVER_H_
#define DRIVACY_IO_WEBSOCKET_SERVER_H_

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "uWebSockets/App.h"

namespace drivacy {
namespace io {
namespace socket {

class WebSocketServer : public AbstractSocket {
 public:
  WebSocketServer(uint32_t party_id, const types::Configuration &config,
                  SocketListener *listener)
      : AbstractSocket(party_id, config, listener) {}

  // Factory function used to simplify construction of inheriting sockets.
  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, const types::Configuration &config,
      SocketListener *listener) {
    return std::make_unique<WebSocketServer>(party_id, config, listener);
  }

  // We can never send queries (or batch sizes) to clients!
  void SendBatch(uint32_t batch_size) override { assert(false); }
  void SendQuery(const types::OutgoingQuery &query) override { assert(false); }

  // We can send responses to clients!
  void SendResponse(const types::Response &response) override;

  // We need to explicitly listen to incoming connections: blocking.
  void Listen() override;

  // Flush is useless, this socket flushes at every send...
  void FlushQueries() override { assert(false); }
  void FlushResponses() override { assert(false); }

 private:
  // Client sockets in order of receiving queries (not connecting).
  std::list<uWS::WebSocket<false, true> *> sockets_;
  std::unordered_map<uWS::WebSocket<false, true> *, uint32_t> socket_counts_;

  // Handle incoming query from a client (parses and then calls QueryListener).
  void HandleQuery(const std::string &msg) const;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_WEBSOCKET_SERVER_H_
