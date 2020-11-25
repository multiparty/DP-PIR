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
  WebSocketClient(uint32_t party_id, uint32_t machine_id,
                  const types::Configuration &config, SocketListener *listener);

  ~WebSocketClient() {
    if (this->socket_->getReadyState() != easywsclient::WebSocket::CLOSED) {
      this->socket_->close();
    }
    delete this->socket_;
  }

  // Factory function used to simplify construction of inheriting sockets.
  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, uint32_t machine_id,
      const types::Configuration &config, SocketListener *listener) {
    return std::make_unique<WebSocketClient>(party_id, machine_id, config,
                                             listener);
  }

  // This is how we send queries to the frontend party!
  void SendQuery(const types::ForwardQuery &query) override;

  // We can never send responses (or batch sizes) as a client.
  void SendBatch(uint32_t batch_size) override { assert(false); }
  void SendResponse(const types::Response &response) override { assert(false); }

  // Start the socket and listen to messages: blocking.
  void Listen() override;

  // Flush is useless, this socket flushes at every send...
  void FlushQueries() override { assert(false); }
  void FlushResponses() override { assert(false); }

 private:
  // The client socket.
  easywsclient::WebSocket *socket_;

  // The total number of queries sent.
  uint32_t queries_sent_count_;

  // Handle incoming responses from server (calls response_listener_).
  void HandleResponse(const std::vector<uint8_t> &msg);
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_WEBSOCKET_CLIENT_H_
