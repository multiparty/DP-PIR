// Copyright 2020 multiparty.org

// The real client-side web-socket interface.
//
// This is only used when running an independent (not a local simulation)
// c++ client, which will use this file to communicate with the
// first frontend server via websockets.

#ifndef DRIVACY_IO_WEBSOCKET_CLIENT_H_
#define DRIVACY_IO_WEBSOCKET_CLIENT_H_

#include <cstdint>
#include <vector>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "third_party/easywsclient/easywsclient.hpp"

namespace drivacy {
namespace io {
namespace socket {

class WebSocketClientListener {
 public:
  virtual void OnReceiveResponse(const types::ForwardResponse &response) = 0;
};

class WebSocketClient {
 public:
  WebSocketClient(uint32_t machine_id, const types::Configuration &config,
                  WebSocketClientListener *listener);

  ~WebSocketClient() {
    if (this->socket_->getReadyState() != easywsclient::WebSocket::CLOSED) {
      this->socket_->close();
    }
    delete this->socket_;
  }

  // We can only send queries from a client.
  void SendQuery(const types::ForwardQuery &query);

  // Start the socket and listen to messages: blocking.
  void Listen();

 private:
  // Configuration.
  WebSocketClientListener *listener_;
  // The client socket.
  easywsclient::WebSocket *socket_;
  // Message sizes.
  uint32_t outgoing_query_msg_size_;
  uint32_t response_msg_size_;
  // The total number of queries sent.
  uint32_t queries_sent_count_;
  // Handle incoming responses from server (calls response_listener_).
  void HandleResponse(const std::vector<uint8_t> &msg);
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_WEBSOCKET_CLIENT_H_
