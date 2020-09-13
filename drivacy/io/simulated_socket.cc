// Copyright 2020 multiparty.org

// A (fake/simulated) socket-like interface for simulating over the wire
// communication, when running all the parties locally for testing/debugging.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#include "drivacy/io/simulated_socket.h"

namespace drivacy {
namespace io {
namespace socket {

// SimulatedSocket.
SimulatedSocket::SimulatedSocket(uint32_t party_id,
                                 QueryListener query_listener,
                                 ResponseListener response_listener,
                                 const types::Configuration &config)
    : AbstractSocket(party_id, query_listener, response_listener, config) {
  assert(SimulatedSocket::sockets_.count(party_id) == 0);
  SimulatedSocket::sockets_.insert({party_id, this});
}

void SimulatedSocket::SendQuery(const types::OutgoingQuery &query) {
  auto [buffer, size] = query.Serialize();
  SimulatedSocket *socket = SimulatedSocket::sockets_.at(this->party_id_ + 1);
  socket->query_listener_(types::IncomingQuery::Deserialize(buffer, size));
}

void SimulatedSocket::SendResponse(const types::Response &response) {
  auto [buffer, _] = response.Serialize();
  SimulatedSocket *socket = SimulatedSocket::sockets_.at(this->party_id_ - 1);
  socket->response_listener_(types::Response::Deserialize(buffer));
}

std::unordered_map<uint32_t, SimulatedSocket *> SimulatedSocket::sockets_ = {};

// SimulatedClientSocket.
SimulatedClientSocket::SimulatedClientSocket(uint32_t party_id,
                                             QueryListener query_listener,
                                             ResponseListener response_listener,
                                             const types::Configuration &config)
    : AbstractSocket(party_id, query_listener, response_listener, config) {
  assert(SimulatedClientSocket::sockets_.count(party_id) == 0);
  SimulatedClientSocket::sockets_.insert({party_id, this});
}

void SimulatedClientSocket::SendQuery(const types::OutgoingQuery &query) {
  assert(this->party_id_ == 0);
  auto [buffer, size] = query.Serialize();
  SimulatedClientSocket *socket = SimulatedClientSocket::sockets_.at(1);
  socket->query_listener_(types::IncomingQuery::Deserialize(buffer, size));
}

void SimulatedClientSocket::SendResponse(const types::Response &response) {
  assert(this->party_id_ == 1);
  auto [buffer, _] = response.Serialize();
  SimulatedClientSocket *socket = SimulatedClientSocket::sockets_.at(0);
  socket->response_listener_(types::Response::Deserialize(buffer));
}

std::unordered_map<uint32_t, SimulatedClientSocket *>
    SimulatedClientSocket::sockets_ = {};

}  // namespace socket
}  // namespace io
}  // namespace drivacy
