// Copyright 2020 multiparty.org

// The real server-side web-socket interface for communicating with client.
// The client side socket is in javascript under //client/.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#include "drivacy/io/websocket_server.h"

#include "uWebSockets/App.h"

namespace drivacy {
namespace io {
namespace socket {

namespace {

// Data attached to each socket upon opening.
struct PerSocketData {};

}  // namespace

// Listening: starts the socket server which spawns new sockets everytime
// a client opens a connection with the server.
void WebSocketServer::Listen() {
  // Set up the socket server.
  int32_t port = this->config_.network()
                     .at(this->party_id_)
                     .machines()
                     .at(this->machine_id_)
                     .webserver_port();
  std::cout << "Listenting to client connections on port " << port << std::endl;
  uWS::App()
      .ws<PerSocketData>("/*", {.message =
                                    [this](auto *ws, std::string_view message,
                                           uWS::OpCode op_code) {
                                      assert(op_code == uWS::OpCode::BINARY);
                                      this->sockets_.push_back(ws);
                                      this->HandleQuery(std::string(message));
                                    }})
      .listen(port, [](auto *token) { assert(token); })
      .run();
}

void WebSocketServer::HandleQuery(const std::string &message) const {
  uint32_t buffer_size =
      types::IncomingQuery::Size(this->party_id_, this->party_count_);
  assert(buffer_size == message.size());
  const unsigned char *buffer =
      reinterpret_cast<const unsigned char *>(message.c_str());
  this->listener_->OnReceiveQuery(
      types::IncomingQuery::Deserialize(buffer, buffer_size));
}

void WebSocketServer::SendResponse(const types::Response &response) {
  auto *ws = this->sockets_.front();
  this->sockets_.pop_front();

  const unsigned char *buffer = response.Serialize();
  ws->send(std::string(reinterpret_cast<const char *>(buffer),
                       this->response_msg_size_),
           uWS::OpCode::BINARY, false);
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
