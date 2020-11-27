// Copyright 2020 multiparty.org

// The real server-side web-socket interface for communicating with client.
// The client side socket is in javascript under //client/.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#include "drivacy/io/websocket_server.h"

#include <cassert>
#include <iostream>

#include "absl/functional/bind_front.h"

namespace drivacy {
namespace io {
namespace socket {

namespace {

// Data attached to each socket upon opening.
struct PerSocketData {};

}  // namespace

// Constructor
WebSocketServer::WebSocketServer(uint32_t party_id, uint32_t machine_id,
                                 const types::Configuration &config,
                                 WebSocketServerListener *listener)
    : party_id_(party_id),
      machine_id_(machine_id),
      config_(config),
      listener_(listener),
      app_(),
      listener_token_(nullptr) {
  // Store configurations.
  this->party_count_ = config.parties();

  // Figure out various message sizes.
  this->incoming_query_msg_size_ =
      types::IncomingQuery::Size(party_id, this->party_count_);
  this->response_msg_size_ = types::Response::Size();

  // Find port.
  this->port_ = this->config_.network()
                    .at(this->party_id_)
                    .machines()
                    .at(this->machine_id_)
                    .webserver_port();
}

// Listening: starts the socket server which spawns new sockets everytime
// a client opens a connection with the server.
void WebSocketServer::Listen() {
  std::cout << "Listenting to client connections on port " << this->port_
            << std::endl;
  // Setup web socket server but do not run it yet!
  this->app_.ws<PerSocketData>(
      "/*", {.message = absl::bind_front(&WebSocketServer::OnMessage, this)});
  this->app_.listen(this->port_, [&](us_listen_socket_t *token) {
    assert(token);
    this->listener_token_ = token;
  });
  this->app_.run();
  std::cout << "Websocket server shut down!" << std::endl;
}

// Shutdown the App.
void WebSocketServer::Stop() {
  std::cout << "Shutting down websocket server..." << std::endl;
  if (this->listener_token_ != nullptr) {
    us_listen_socket_close(0, this->listener_token_);
  }
}

void WebSocketServer::OnMessage(uWS::WebSocket<false, true> *ws,
                                std::string_view message, uWS::OpCode op_code) {
  assert(op_code == uWS::OpCode::BINARY);
  this->sockets_.push_back(ws);
  this->HandleQuery(std::string(message));
}

void WebSocketServer::HandleQuery(const std::string &message) const {
  assert(this->incoming_query_msg_size_ == message.size());
  const unsigned char *buffer =
      reinterpret_cast<const unsigned char *>(message.c_str());
  this->listener_->OnReceiveQuery(types::IncomingQuery::Deserialize(
      buffer, this->incoming_query_msg_size_));
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
