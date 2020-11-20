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

void SimulatedSocket::SendQuery(const types::ForwardQuery &query) {
  SimulatedSocket *socket = SimulatedSocket::sockets_.at(this->party_id_ + 1);
  socket->listener_->OnReceiveQuery(
      types::IncomingQuery::Deserialize(query, this->outgoing_query_msg_size_));
}

void SimulatedSocket::SendResponse(const types::Response &response) {
  types::ForwardResponse forward = response.Serialize();
  SimulatedSocket *socket = SimulatedSocket::sockets_.at(this->party_id_ - 1);
  socket->listener_->OnReceiveResponse(forward);
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

void SimulatedClientSocket::SendQuery(const types::ForwardQuery &query) {
  assert(this->party_id_ == 0);
  SimulatedClientSocket *socket = SimulatedClientSocket::sockets_.at(1);
  socket->listener_->OnReceiveQuery(
      types::IncomingQuery::Deserialize(query, this->outgoing_query_msg_size_));
}

void SimulatedClientSocket::SendResponse(const types::Response &response) {
  assert(this->party_id_ == 1);
  types::ForwardResponse forward = response.Serialize();
  SimulatedClientSocket *socket = SimulatedClientSocket::sockets_.at(0);
  socket->listener_->OnReceiveResponse(forward);
}

std::unordered_map<uint32_t, SimulatedClientSocket *>
    SimulatedClientSocket::sockets_ = {};

// SimulatedIntraPartySocket.
SimulatedIntraPartySocket::SimulatedIntraPartySocket(
    uint32_t party_id, uint32_t machine_id, const types::Configuration &config,
    IntraPartySocketListener *listener)
    : AbstractIntraPartySocket(party_id, machine_id, config, listener) {
  // First, make sure party_id has a nested map.
  if (SimulatedIntraPartySocket::sockets_.count(party_id) == 0) {
    SimulatedIntraPartySocket::sockets_.insert({party_id, {}});
  }
  // Insert this simulated socket into the nested map.
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_.at(party_id);
  assert(nested_map.count(machine_id) == 0);
  nested_map.insert({machine_id, this});
}

void SimulatedIntraPartySocket::SendQuery(uint32_t machine_id,
                                          const types::OutgoingQuery &query) {
  types::ForwardQuery forward = query.Serialize();
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_.at(this->party_id_);
  SimulatedIntraPartySocket *socket = nested_map.at(machine_id);
  socket->listener_->OnReceiveQuery(this->machine_id_, forward);
}

void SimulatedIntraPartySocket::SendResponse(
    uint32_t machine_id, const types::ForwardResponse &response) {
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_.at(this->party_id_);
  SimulatedIntraPartySocket *socket = nested_map.at(machine_id);
  socket->listener_->OnReceiveResponse(this->machine_id_,
                                       types::Response::Deserialize(response));
}

std::unordered_map<uint32_t,
                   std::unordered_map<uint32_t, SimulatedIntraPartySocket *>>
    SimulatedIntraPartySocket::sockets_ = {};

}  // namespace socket
}  // namespace io
}  // namespace drivacy
