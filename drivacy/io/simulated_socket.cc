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
SimulatedSocket::SimulatedSocket(uint32_t party_id, uint32_t machine_id,
                                 const types::Configuration &config,
                                 SocketListener *listener)
    : AbstractSocket(party_id, machine_id, config, listener) {
  assert(SimulatedSocket::sockets_.count(party_id) == 0);
  SimulatedSocket::sockets_.insert({party_id, this});
}

void SimulatedSocket::SendBatch(uint32_t batch_size) {
  SimulatedSocket *socket = SimulatedSocket::sockets_.at(this->party_id_ + 1);
  socket->listener_->OnReceiveBatch(batch_size);
}

void SimulatedSocket::SendQuery(const types::OutgoingQuery &query) {
  const unsigned char *buffer = query.Serialize();
  SimulatedSocket *socket = SimulatedSocket::sockets_.at(this->party_id_ + 1);
  socket->listener_->OnReceiveQuery(types::IncomingQuery::Deserialize(
      buffer, this->outgoing_query_msg_size_));
}

void SimulatedSocket::SendResponse(const types::Response &response) {
  const unsigned char *buffer = response.Serialize();
  SimulatedSocket *socket = SimulatedSocket::sockets_.at(this->party_id_ - 1);
  socket->listener_->OnReceiveResponse(types::Response::Deserialize(buffer));
}

std::unordered_map<uint32_t, SimulatedSocket *> SimulatedSocket::sockets_ = {};

// SimulatedClientSocket.
SimulatedClientSocket::SimulatedClientSocket(uint32_t party_id,
                                             uint32_t machine_id,
                                             const types::Configuration &config,
                                             SocketListener *listener)
    : AbstractSocket(party_id, machine_id, config, listener) {
  assert(SimulatedClientSocket::sockets_.count(party_id) == 0);
  SimulatedClientSocket::sockets_.insert({party_id, this});
}

void SimulatedClientSocket::SendQuery(const types::OutgoingQuery &query) {
  assert(this->party_id_ == 0);
  const unsigned char *buffer = query.Serialize();
  SimulatedClientSocket *socket = SimulatedClientSocket::sockets_.at(1);
  socket->listener_->OnReceiveQuery(types::IncomingQuery::Deserialize(
      buffer, this->outgoing_query_msg_size_));
}

void SimulatedClientSocket::SendResponse(const types::Response &response) {
  assert(this->party_id_ == 1);
  const unsigned char *buffer = response.Serialize();
  SimulatedClientSocket *socket = SimulatedClientSocket::sockets_.at(0);
  socket->listener_->OnReceiveResponse(types::Response::Deserialize(buffer));
}

std::unordered_map<uint32_t, SimulatedClientSocket *>
    SimulatedClientSocket::sockets_ = {};

}  // namespace socket
}  // namespace io
}  // namespace drivacy
