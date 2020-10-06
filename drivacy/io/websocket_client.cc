// Copyright 2020 multiparty.org

// The real client-side web-socket interface.
//
// This is only used when running an independent (not a local simulation)
// c++ client, which will use this file to communicate with the
// first frontend server via websockets.

#include "drivacy/io/websocket_client.h"

#include "absl/functional/bind_front.h"
#include "absl/strings/str_format.h"

namespace drivacy {
namespace io {
namespace socket {

WebSocketClient::WebSocketClient(uint32_t party_id,
                                 const types::Configuration &config,
                                 SocketListener *listener)
    : AbstractSocket(party_id, config, listener) {
  // Find address of first frontend.
  uint32_t port = this->config_.network().at(1).webserver_port();
  const std::string &ip = this->config_.network().at(1).ip();
  std::string address = absl::StrFormat("ws://%s:%d", ip, port);

  // Create socket.
  this->socket_ = easywsclient::WebSocket::from_url(address);
  assert(this->socket_);
}

void WebSocketClient::Listen() {
  // Block until a response is heard.
  while (this->socket_->getReadyState() != easywsclient::WebSocket::CLOSED) {
    this->socket_->poll(-1);
    this->socket_->dispatchBinary(
        absl::bind_front(&WebSocketClient::HandleResponse, this));
  }
}

void WebSocketClient::SendQuery(const types::OutgoingQuery &query) {
  const unsigned char *buffer = query.Serialize();
  std::string msg(reinterpret_cast<const char *>(buffer),
                  this->query_msg_size_);
  this->socket_->sendBinary(msg);
  this->socket_->poll();
}

void WebSocketClient::HandleResponse(const std::vector<uint8_t> &msg) const {
  const unsigned char *buffer = &(msg[0]);
  types::Response response = types::Response::Deserialize(buffer);
  this->listener_->OnReceiveResponse(response);
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
