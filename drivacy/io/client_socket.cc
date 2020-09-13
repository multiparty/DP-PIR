// Copyright 2020 multiparty.org

// The real server-side web-socket interface for communicating with client.
// The client side socket is in javascript under //client/.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#include "drivacy/io/client_socket.h"

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
// This function never returns until ClientSocket::Close() is called.
void ClientSocket::Listen() {
  // Set up the socket server.
  int32_t port = this->config_.network().at(this->party_id_).webserver_port();
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

void ClientSocket::HandleQuery(const std::string &message) const {
  uint32_t buffer_size =
      types::IncomingQuery::Size(this->party_id_, this->party_count_);
  assert(buffer_size == message.size());
  const unsigned char *buffer =
      reinterpret_cast<const unsigned char *>(message.c_str());
  this->query_listener_(types::IncomingQuery::Deserialize(buffer, buffer_size));
}

void ClientSocket::SendResponse(const types::Response &response) {
  auto *ws = this->sockets_.front();
  auto [buffer, size] = response.Serialize();
  ws->send(std::string(reinterpret_cast<const char *>(buffer), size),
           uWS::OpCode::BINARY, false);
  ws->close();
  this->sockets_.pop_front();
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
