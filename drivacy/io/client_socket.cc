// Copyright 2020 multiparty.org

// The real server-side web-socket interface for communicating with client.
// The client side socket is in javascript under //client/.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#include "drivacy/io/client_socket.h"

#include "drivacy/protocol/client.h"
#include "uWebSockets/App.h"

namespace drivacy {
namespace io {
namespace socket {

void ClientSocket::Listen() {
  // Set up the socket server.
  uWS::App()
      .ws<PerSocketData>(
          "/*",
          {.open =
               [this](auto *ws) {
                 uint32_t client_id = this->client_counter_++;
                 static_cast<PerSocketData *>(ws->getUserData())->client_id =
                     client_id;
                 this->sockets_.insert({client_id, ws});
               },
           .message =
               [this](auto *ws, std::string_view message, uWS::OpCode op_code) {
                 assert(op_code == uWS::OpCode::TEXT);
                 this->HandleQuery(
                     std::string(message),
                     static_cast<PerSocketData *>(ws->getUserData()));
               },
           .close =
               [this](auto *ws, int code, std::string_view message) {
                 uint32_t client_id =
                     static_cast<PerSocketData *>(ws->getUserData())->client_id;
                 this->sockets_.erase(client_id);
               }})
      .listen(3000, [](auto *token) { assert(token); })
      .run();
}

void ClientSocket::HandleQuery(std::string message, PerSocketData *data) const {
  uint64_t value = std::stoull(message);
  types::Query query =
      protocol::client::CreateQuery(value, this->config_, &data->state);
  data->tag = query.tag();
  query.set_tag(data->client_id);
  this->query_listener_(data->client_id, query);
}

void ClientSocket::SendResponse(uint32_t client,
                                const types::Response &response) const {
  auto *ws = this->sockets_.at(client);
  PerSocketData *data = static_cast<PerSocketData *>(ws->getUserData());
  types::Response response_ = response;
  response_.set_tag(data->tag);
  const auto &[query, value] =
      protocol::client::ReconstructResponse(response_, &data->state);
  ws->send(std::to_string(value), uWS::OpCode::TEXT, false);
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
