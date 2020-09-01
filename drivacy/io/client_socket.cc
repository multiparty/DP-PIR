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

struct PerSocketData {
  uint32_t client_id;
  uint64_t tag;
};

}  // namespace

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
                 assert(op_code == uWS::OpCode::BINARY);
                 PerSocketData *data =
                     static_cast<PerSocketData *>(ws->getUserData());
                 this->HandleQuery(std::string(message), data->client_id);
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

void ClientSocket::HandleQuery(std::string message, uint32_t client_id) const {
  // Parse protobuf query.
  types::Query query;
  assert(query.ParseFromString(message));
  // Set the client id as the tag.
  query.set_tag(client_id);
  this->query_listener_(client_id, query);
}

void ClientSocket::SendResponse(uint32_t client_id,
                                const types::Response &response) const {
  auto *ws = this->sockets_.at(client_id);
  PerSocketData *data = static_cast<PerSocketData *>(ws->getUserData());
  types::Response response_ = response;
  response_.set_tag(data->tag);

  std::string bits;
  assert(response_.SerializeToString(&bits));
  ws->send(bits, uWS::OpCode::BINARY, false);
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
