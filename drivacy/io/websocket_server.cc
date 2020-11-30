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
#include "drivacy/primitives/crypto.h"

namespace drivacy {
namespace io {
namespace socket {

namespace {

// Data attached to each socket upon opening.
struct PerSocketData {};

}  // namespace

// Constructor
WebSocketServer::WebSocketServer(uint32_t party_id, uint32_t machine_id,
                                 bool online,
                                 const types::Configuration &config,
                                 WebSocketServerListener *listener)
    : party_id_(party_id),
      machine_id_(machine_id),
      online_(online),
      config_(config),
      listener_(listener),
      app_(),
      listener_token_(nullptr) {
  // Store configurations.
  this->party_count_ = config.parties();

  // Figure out various message sizes.
  this->message_size_ =
      primitives::crypto::OnionCipherSize(party_id, this->party_count_);

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
  if (this->online_) {
    this->sockets_.push_back(ws);
  }
  this->Handle(std::string(message));
}

void WebSocketServer::Handle(const std::string &message) const {
  if (this->online_) {
    assert(sizeof(types::Query) == message.size());
    this->listener_->OnReceiveQuery(
        *reinterpret_cast<const types::Query *>(message.c_str()));
  } else {
    assert(this->message_size_ == message.size());
    this->listener_->OnReceiveMessage(
        reinterpret_cast<types::CipherText>(message.c_str()));
  }
}

void WebSocketServer::SendResponse(const types::Response &response) {
  assert(this->online_);
  auto *ws = this->sockets_.front();
  this->sockets_.pop_front();
  ws->send(std::string(reinterpret_cast<const char *>(&response),
                       sizeof(types::Response)),
           uWS::OpCode::BINARY, false);
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
