// Copyright 2020 multiparty.org

// A (fake/simulated) socket-like interface for simulating over the wire
// communication, when running all the parties locally for testing/debugging.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#include "drivacy/io/simulated_socket.h"

#include <cstring>

namespace drivacy {
namespace io {
namespace socket {

// SimulatedSocket.
SimulatedSocket::SimulatedSocket(uint32_t party_id, uint32_t machine_id,
                                 const types::Configuration &config,
                                 SocketListener *listener)
    : AbstractSocket(party_id, machine_id, config, listener) {
  std::unordered_map<uint32_t, SimulatedSocket *> &nested_map =
      SimulatedSocket::sockets_[party_id];
  assert(nested_map.count(machine_id) == 0);
  nested_map.insert({machine_id, this});
}

void SimulatedSocket::SendBatch(uint32_t batch_size) {
  SimulatedSocket *socket =
      SimulatedSocket::sockets_.at(this->party_id_ + 1).at(this->machine_id_);
  socket->listener_->OnReceiveBatchSize(batch_size);
}

void SimulatedSocket::SendQuery(const types::ForwardQuery &query) {
  // Make a copy, simulates sending over a network for purposes of memory
  // management.
  unsigned char *buffer = new unsigned char[this->outgoing_query_msg_size_];
  memcpy(buffer, query, this->outgoing_query_msg_size_);
  // Pass copy to party.
  SimulatedSocket *socket =
      SimulatedSocket::sockets_.at(this->party_id_ + 1).at(this->machine_id_);
  socket->listener_->OnReceiveQuery(types::IncomingQuery::Deserialize(
      buffer, this->outgoing_query_msg_size_));
  // Free memory.
  delete[] buffer;
}

void SimulatedSocket::SendResponse(const types::Response &response) {
  types::ForwardResponse forward = response.Serialize();
  // Make a copy, simulates sending over a network for purposes of memory
  // management.
  unsigned char *buffer = new unsigned char[this->response_msg_size_];
  memcpy(buffer, forward, this->response_msg_size_);
  // Pass copy to party.
  SimulatedSocket *socket =
      SimulatedSocket::sockets_.at(this->party_id_ - 1).at(this->machine_id_);
  socket->listener_->OnReceiveResponse(buffer);
  // Free memory.
  delete[] buffer;
}

std::unordered_map<uint32_t, std::unordered_map<uint32_t, SimulatedSocket *>>
    SimulatedSocket::sockets_ = {};

// SimulatedClientSocket.
SimulatedClientSocket::SimulatedClientSocket(uint32_t party_id,
                                             uint32_t machine_id,
                                             const types::Configuration &config,
                                             SocketListener *listener)
    : AbstractSocket(party_id, machine_id, config, listener) {
  std::unordered_map<uint32_t, SimulatedClientSocket *> &nested_map =
      SimulatedClientSocket::sockets_[party_id];
  assert(nested_map.count(machine_id) == 0);
  nested_map.insert({machine_id, this});
}

void SimulatedClientSocket::SendQuery(const types::ForwardQuery &query) {
  assert(this->party_id_ == 0);
  // Make a copy, simulates sending over a network for purposes of memory
  // management.
  unsigned char *buffer = new unsigned char[this->outgoing_query_msg_size_];
  memcpy(buffer, query, this->outgoing_query_msg_size_);
  // Pass copy over to party.
  SimulatedClientSocket *socket =
      SimulatedClientSocket::sockets_.at(1).at(this->machine_id_);
  socket->listener_->OnReceiveQuery(types::IncomingQuery::Deserialize(
      buffer, this->outgoing_query_msg_size_));
  // Free copy: it is the socket's responsibility to manage memory it allocates.
  delete[] buffer;
}

void SimulatedClientSocket::SendResponse(const types::Response &response) {
  assert(this->party_id_ == 1);
  types::ForwardResponse forward = response.Serialize();
  // Make a copy, simulates sending over a network for purposes of memory
  // management.
  unsigned char *buffer = new unsigned char[this->response_msg_size_];
  memcpy(buffer, forward, this->response_msg_size_);
  // Pass response to party.
  SimulatedClientSocket *socket =
      SimulatedClientSocket::sockets_.at(0).at(this->machine_id_);
  socket->listener_->OnReceiveResponse(buffer);
  // Free memory.
  delete[] buffer;
}

std::unordered_map<uint32_t,
                   std::unordered_map<uint32_t, SimulatedClientSocket *>>
    SimulatedClientSocket::sockets_ = {};

// SimulatedIntraPartySocket.
SimulatedIntraPartySocket::SimulatedIntraPartySocket(
    uint32_t party_id, uint32_t machine_id, const types::Configuration &config,
    IntraPartySocketListener *listener)
    : AbstractIntraPartySocket(party_id, machine_id, config, listener) {
  // Insert this simulated socket into the nested map.
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_[party_id];
  assert(nested_map.count(machine_id) == 0);
  nested_map.insert({machine_id, this});
}

void SimulatedIntraPartySocket::BroadcastBatchSize(uint32_t batch_size) {
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_.at(this->party_id_);
  bool should_continue = true;
  for (auto [_, socket] : nested_map) {
    should_continue &=
        socket->listener_->OnReceiveBatchSize(this->machine_id_, batch_size);
  }
  if (should_continue) {
    for (auto [_, socket] : nested_map) {
      socket->listener_->OnReceiveBatchSize2();
    }
  }
}

void SimulatedIntraPartySocket::BroadcastQueriesReady() {
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_.at(this->party_id_);
  for (auto [_, socket] : nested_map) {
    socket->listener_->OnQueriesReady(this->machine_id_);
  }
}

void SimulatedIntraPartySocket::BroadcastResponsesReady() {
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_.at(this->party_id_);
  for (auto [_, socket] : nested_map) {
    socket->listener_->OnResponsesReady(this->machine_id_);
  }
}

void SimulatedIntraPartySocket::SendQuery(uint32_t machine_id,
                                          const types::OutgoingQuery &query) {
  types::ForwardQuery forward = query.Serialize();
  // Make a copy, simulates sending over a network for purposes of memory
  // management.
  unsigned char *buffer = new unsigned char[this->query_msg_size_];
  memcpy(buffer, forward, this->query_msg_size_);
  // Pass over copy to party.
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_.at(this->party_id_);
  SimulatedIntraPartySocket *socket = nested_map.at(machine_id);
  socket->listener_->OnReceiveQuery(this->machine_id_, buffer);
  // Free memory.
  delete[] buffer;
}

void SimulatedIntraPartySocket::SendResponse(
    uint32_t machine_id, const types::ForwardResponse &response) {
  // Make a copy, simulates sending over a network for purposes of memory
  // management.
  unsigned char *buffer = new unsigned char[this->response_msg_size_];
  memcpy(buffer, response, this->response_msg_size_);
  // Pass over copy to party.
  std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
      SimulatedIntraPartySocket::sockets_.at(this->party_id_);
  SimulatedIntraPartySocket *socket = nested_map.at(machine_id);
  socket->listener_->OnReceiveResponse(this->machine_id_,
                                       types::Response::Deserialize(buffer));
  // Free memory.
  delete[] buffer;
}

std::unordered_map<uint32_t,
                   std::unordered_map<uint32_t, SimulatedIntraPartySocket *>>
    SimulatedIntraPartySocket::sockets_ = {};

}  // namespace socket
}  // namespace io
}  // namespace drivacy
