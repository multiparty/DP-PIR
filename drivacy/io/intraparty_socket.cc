// Copyright 2020 multiparty.org

// Socket Interface responsible for intra-party communication over TCP.
//
// Machine with id i acts a server for all sockets with other machines with
// IDs less than i.

#include "drivacy/io/intraparty_socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>

namespace drivacy {
namespace io {
namespace socket {

namespace {

inline void ReadUntil(int socket, unsigned char *buffer, uint32_t size) {
  uint32_t bytes = 0;
  while (bytes < size) {
    bytes += read(socket, buffer + bytes, size - bytes);
  }
}

inline void SendAssert(int socket, const unsigned char *buffer, uint32_t size) {
  assert(send(socket, buffer, size, 0) == size);
}

int ServerSocket(const types::Configuration &config, uint32_t party_id,
                 uint32_t machine_id) {
  const auto &machines = config.network().at(party_id).machines();
  int32_t port = machines.at(machine_id).intraparty_port();

  // Creating socket file descriptor.
  int re = 1;
  int serverfd = ::socket(AF_INET, SOCK_STREAM, 0);
  assert(serverfd >= 0);
  assert(setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) >= 0);

  // Filling server information.
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;  // IPv4
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(port);

  // Bind the socket to the port address.
  assert(bind(serverfd, reinterpret_cast<struct sockaddr *>(&servaddr),
              sizeof(servaddr)) >= 0);
  return serverfd;
}

void AcceptClients(int serverfd, std::vector<int> *map,
                   uint32_t clients_count) {
  // Listen for only 1 connection.
  std::cout << "Waiting for " << clients_count << " intraparty connections..."
            << std::endl;
  assert(listen(serverfd, clients_count) >= 0);

  // Accept the connection.
  struct sockaddr_in servaddr;
  auto *servaddr_ptr = reinterpret_cast<sockaddr *>(&servaddr);
  auto servaddr_len = static_cast<socklen_t>(sizeof(servaddr));
  uint32_t machine_id;
  while (clients_count-- > 0) {
    // Wait for client to connect.
    int clientfd = accept(serverfd, servaddr_ptr, &servaddr_len);
    assert(clientfd >= 0);
    // Listen to the client's machine id.
    ReadUntil(clientfd, reinterpret_cast<unsigned char *>(&machine_id),
              sizeof(machine_id));
    // Store socket.
    (*map)[machine_id] = clientfd;
    std::cout << "Machine " << machine_id << " connected. " << clients_count
              << " remain!" << std::endl;
  }

  // Done!
  std::cout << "All intraparty clients connected!" << std::endl;
}

int ClientSocket(const types::Configuration &config, uint32_t party_id,
                 uint32_t machine_id, uint32_t target_machine_id) {
  // Read configurations.
  const auto &machines = config.network().at(party_id).machines();
  const std::string &ip = machines.at(target_machine_id).ip();
  int32_t port = machines.at(target_machine_id).intraparty_port();

  // Creating socket file descriptor.
  int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
  assert(sockfd >= 0);

  // Filling server information.
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;  // IPv4
  servaddr.sin_port = htons(port);
  assert(inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) >= 0);

  // Connect to the server.
  std::cout << "Connecting to machine " << target_machine_id << "..."
            << std::endl;
  while (connect(sockfd, reinterpret_cast<sockaddr *>(&servaddr),
                 sizeof(servaddr)) < 0) {
    sleep(1);
  }

  // Send machine id to server.
  SendAssert(sockfd, reinterpret_cast<unsigned char *>(&machine_id),
             sizeof(uint32_t));

  // Done!
  std::cout << "Connected!" << std::endl;
  return sockfd;
}

}  // namespace

// Constructor.
IntraPartyTCPSocket::IntraPartyTCPSocket(uint32_t party_id, uint32_t machine_id,
                                         const types::Configuration &config,
                                         IntraPartySocketListener *listener)
    : party_id_(party_id),
      machine_id_(machine_id),
      config_(config),
      listener_(listener) {
  // Store configuration parameters.
  this->party_count_ = config.parties();
  this->parallelism_ = config.parallelism();
  this->query_msg_size_ = types::ForwardQuerySize(party_id, this->party_count_);
  this->response_msg_size_ = types::ForwardResponseSize();
  // Create sockets_ map
  this->sockets_.resize(this->parallelism_ + 1, -1);
  // Input/read buffers.
  this->read_query_buffer_ = new unsigned char[this->query_msg_size_];
  this->read_response_buffer_ = new unsigned char[this->response_msg_size_];
  // Counters.
  this->incoming_query_counts_.resize(this->parallelism_ + 1, 0);
  this->incoming_response_counts_.resize(this->parallelism_ + 1, 0);
  this->sockets_with_queries_ = 0;
  this->sockets_with_responses_ = 0;
  // Setup sockets.
  // The backend party does not need Intra-party sockets.
  if (this->party_id_ < this->party_count_) {
    if (machine_id - 1 > 0) {
      int serverfd = ServerSocket(config, party_id, machine_id);
      AcceptClients(serverfd, &this->sockets_, machine_id - 1);
    }
    for (uint32_t m = machine_id + 1; m <= this->parallelism_; m++) {
      this->sockets_[m] = ClientSocket(config, party_id, machine_id, m);
    }
  }
}

// Destructor.
IntraPartyTCPSocket::~IntraPartyTCPSocket() {
  // Free all open sockets!
  for (int socket : this->sockets_) {
    if (socket > -1) {
      close(socket);
    }
  }
  // Delete read buffers.
  delete[] this->read_query_buffer_;
  delete[] this->read_response_buffer_;
}

// Set expected query counts.
void IntraPartyTCPSocket::SetQueryCounts(
    std::vector<uint32_t> &&incoming_query_counts) {
  this->incoming_query_counts_ = incoming_query_counts;
  this->sockets_with_queries_ = 0;
  for (uint32_t count : this->incoming_query_counts_) {
    if (count > 0) {
      this->sockets_with_queries_++;
    }
  }
}

// Configure poll(...).
uint32_t IntraPartyTCPSocket::FdCount() { return this->parallelism_ - 1; }
bool IntraPartyTCPSocket::PollQueries(pollfd *fds) {
  for (uint32_t m = 1; m < this->machine_id_; m++) {
    fds[m - 1].revents = 0;
    if (this->incoming_query_counts_.at(m) > 0) {
      fds[m - 1].fd = this->sockets_.at(m);
      fds[m - 1].events = POLLIN;
    } else {
      fds[m - 1].fd = -1;
      fds[m - 1].events = 0;
    }
  }
  for (uint32_t m = this->machine_id_ + 1; m <= this->parallelism_; m++) {
    fds[m - 2].revents = 0;
    if (this->incoming_query_counts_.at(m) > 0) {
      fds[m - 2].fd = this->sockets_.at(m);
      fds[m - 2].events = POLLIN;
    } else {
      fds[m - 2].fd = -1;
      fds[m - 2].events = 0;
    }
  }
  return this->sockets_with_queries_ == 0;
}
bool IntraPartyTCPSocket::PollResponses(pollfd *fds) {
  for (uint32_t m = 1; m < this->machine_id_; m++) {
    fds[m - 1].revents = 0;
    if (this->incoming_response_counts_.at(m) > 0) {
      fds[m - 1].fd = this->sockets_.at(m);
      fds[m - 1].events = POLLIN;
    } else {
      fds[m - 1].fd = -1;
      fds[m - 1].events = 0;
    }
  }
  for (uint32_t m = this->machine_id_ + 1; m <= this->parallelism_; m++) {
    fds[m - 2].revents = 0;
    if (this->incoming_response_counts_.at(m) > 0) {
      fds[m - 2].fd = this->sockets_.at(m);
      fds[m - 2].events = POLLIN;
    } else {
      fds[m - 2].fd = -1;
      fds[m - 2].events = 0;
    }
  }
  return this->sockets_with_responses_ == 0;
}

// Blocking reads.
bool IntraPartyTCPSocket::ReadQuery(uint32_t fd_index, pollfd *fds) {
  // Translate index to a machine id!
  uint32_t machine_id = fd_index + 1;
  if (machine_id >= this->machine_id_) {
    machine_id++;
  }
  assert(machine_id <= this->parallelism_);
  assert(machine_id != this->machine_id_);
  assert(machine_id > 0);
  assert(this->incoming_query_counts_.at(machine_id) > 0);

  // Blocking read.
  ReadUntil(this->sockets_.at(machine_id), this->read_query_buffer_,
            this->query_msg_size_);

  // Update fds.
  bool done = false;
  if (--this->incoming_query_counts_.at(machine_id) == 0) {
    fds[fd_index].fd = -1;
    fds[fd_index].events = 0;
    fds[fd_index].revents = 0;
    done = (--this->sockets_with_queries_ == 0);
  }

  // Call event handler.
  this->listener_->OnReceiveQuery(machine_id, this->read_query_buffer_);
  return done;
}
bool IntraPartyTCPSocket::ReadResponse(uint32_t fd_index, pollfd *fds) {
  // Translate index to a machine id!
  uint32_t machine_id = fd_index + 1;
  if (machine_id >= this->machine_id_) {
    machine_id++;
  }
  assert(machine_id <= this->parallelism_);
  assert(machine_id != this->machine_id_);
  assert(machine_id > 0);
  assert(this->incoming_response_counts_.at(machine_id) > 0);

  // Blocking read.
  ReadUntil(this->sockets_.at(machine_id), this->read_response_buffer_,
            this->response_msg_size_);

  // Update fds.
  bool done = false;
  if (--this->incoming_response_counts_.at(machine_id) == 0) {
    fds[fd_index].fd = -1;
    fds[fd_index].events = 0;
    fds[fd_index].revents = 0;
    done = (--this->sockets_with_responses_ == 0);
  }

  // Call event handler.
  this->listener_->OnReceiveResponse(
      machine_id, types::Response::Deserialize(this->read_response_buffer_));
  return done;
}

// Broadcast collection (blocking).
void IntraPartyTCPSocket::CollectBatchSizes() {
  uint32_t batch_size = 0;
  for (uint32_t m = 1; m <= this->parallelism_; m++) {
    if (m != this->machine_id_) {
      ReadUntil(this->sockets_.at(m),
                reinterpret_cast<unsigned char *>(&batch_size),
                sizeof(batch_size));
      this->listener_->OnReceiveBatchSize(m, batch_size);
    }
  }
  this->listener_->OnCollectedBatchSizes();
}
void IntraPartyTCPSocket::CollectQueriesReady() {
  unsigned char ready = 0;
  for (uint32_t m = 1; m <= this->parallelism_; m++) {
    if (m != this->machine_id_) {
      ReadUntil(this->sockets_.at(m), reinterpret_cast<unsigned char *>(&ready),
                sizeof(ready));
      assert(ready == 1);
      ready = 0;
    }
  }
}
void IntraPartyTCPSocket::CollectResponsesReady() {
  unsigned char ready = 0;
  for (uint32_t m = 1; m <= this->parallelism_; m++) {
    if (m != this->machine_id_) {
      ReadUntil(this->sockets_.at(m), reinterpret_cast<unsigned char *>(&ready),
                sizeof(ready));
      assert(ready == 1);
      ready = 0;
    }
  }
}

// Broadcasts.
void IntraPartyTCPSocket::BroadcastBatchSize(uint32_t batch_size) {
  for (int socket : this->sockets_) {
    if (socket > -1) {
      SendAssert(socket, reinterpret_cast<unsigned char *>(&batch_size),
                 sizeof(batch_size));
    }
  }
  this->listener_->OnReceiveBatchSize(this->machine_id_, batch_size);
}
void IntraPartyTCPSocket::BroadcastQueriesReady() {
  unsigned char msg = 1;
  for (int socket : this->sockets_) {
    if (socket > -1) {
      SendAssert(socket, reinterpret_cast<unsigned char *>(&msg), sizeof(msg));
    }
  }
}
void IntraPartyTCPSocket::BroadcastResponsesReady() {
  unsigned char msg = 1;
  for (int socket : this->sockets_) {
    if (socket > -1) {
      SendAssert(socket, reinterpret_cast<unsigned char *>(&msg), sizeof(msg));
    }
  }
}

// Sending queries and responses.
void IntraPartyTCPSocket::SendQuery(uint32_t machine_id,
                                    const types::OutgoingQuery &query) {
  types::ForwardQuery serialized = query.Serialize();
  if (machine_id == this->machine_id_) {
    this->listener_->OnReceiveQuery(machine_id, serialized);
  } else {
    // Track how many queries are sent to a machine, we expect an equal
    // number of responses to come back to us form that machine.
    uint32_t &count = this->incoming_response_counts_.at(machine_id);
    if (count == 0) {
      this->sockets_with_responses_++;
    }
    count++;
    // Send over the socket.
    int socket = this->sockets_.at(machine_id);
    SendAssert(socket, serialized, this->query_msg_size_);
  }
}
void IntraPartyTCPSocket::SendResponse(uint32_t machine_id,
                                       const types::ForwardResponse &response) {
  if (machine_id == this->machine_id_) {
    types::Response deserialized = types::Response::Deserialize(response);
    this->listener_->OnReceiveResponse(machine_id, deserialized);
  } else {
    int socket = this->sockets_.at(machine_id);
    SendAssert(socket, response, this->response_msg_size_);
  }
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
