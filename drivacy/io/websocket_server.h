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
#include <string>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"
#include "uWebSockets/App.h"

namespace drivacy {
namespace io {
namespace socket {

class WebSocketServerListener {
 public:
  // Handlers for when a query or response are received.
  virtual void OnReceiveMessage(const types::CipherText &message) = 0;
  virtual void OnReceiveQuery(const types::Query &query) = 0;
};

class WebSocketServer {
 public:
  WebSocketServer(uint32_t party_id, uint32_t machine_id, bool online,
                  const types::Configuration &config,
                  WebSocketServerListener *listener);

  // We need to explicitly listen to incoming connections: blocking.
  void Listen();
  void Stop();

  // We can only send responses to the clients.
  void SendResponse(const types::Response &response);

 private:
  // Configurations.
  uint32_t party_id_;
  uint32_t machine_id_;
  bool online_;
  uint32_t party_count_;
  types::Configuration config_;
  WebSocketServerListener *listener_;
  // Message sizes.
  uint32_t message_size_;
  // Client sockets in order of receiving queries (not connecting).
  std::list<uWS::WebSocket<false, true> *> sockets_;
  uWS::App app_;
  us_listen_socket_t *listener_token_;
  uint32_t port_;
  // Handle incoming query from a client (parses and then calls QueryListener).
  void OnMessage(uWS::WebSocket<false, true> *ws, std::string_view message,
                 uWS::OpCode op_code);
  void Handle(const std::string &msg) const;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_WEBSOCKET_SERVER_H_
