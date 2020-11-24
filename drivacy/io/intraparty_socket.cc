// Copyright 2020 multiparty.org

// Socket Interface responsible for intra-party communication over TCP.
//
// Machine with id i acts a server for all sockets with other machines with
// IDs less than i.

#include "drivacy/io/intraparty_socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
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
#include <utility>

#include "absl/functional/bind_front.h"

namespace drivacy {
namespace io {
namespace socket {

namespace {

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
    assert(read(clientfd, &machine_id, sizeof(uint32_t)) == sizeof(uint32_t));
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
  assert(send(sockfd, &machine_id, sizeof(uint32_t), 0) == sizeof(uint32_t));

  // Done!
  std::cout << "Connected!" << std::endl;
  return sockfd;
}

void FillPoll(pollfd *fds, const std::vector<int> &sockets,
              const std::vector<uint32_t> &counts, uint32_t machine_id) {
  for (uint32_t m = 1; m < sockets.size(); m++) {
    if (m != machine_id && counts.at(m) > 0) {
      fds[m - 1].fd = sockets.at(m);
      fds[m - 1].events = POLLIN;
      fds[m - 1].revents = 0;
    } else {
      fds[m - 1].fd = -1;
      fds[m - 1].events = 0;
      fds[m - 1].revents = 0;
    }
  }
}

uint32_t PollAndFind(pollfd *fds, nfds_t nfds) {
  // Poll for POLLIN event.
  assert(poll(fds, nfds, -1) > 0);
  // Find socket that has POLLIN ready.
  for (uint32_t m = 1; m <= nfds; m++) {
    if (fds[m - 1].revents & POLLIN) {
      return m;
    }
  }
  assert(false);
}

}  // namespace

// Constructor.
IntraPartyTCPSocket::IntraPartyTCPSocket(uint32_t party_id, uint32_t machine_id,
                                         const types::Configuration &config,
                                         IntraPartySocketListener *listener)
    : AbstractIntraPartySocket(party_id, machine_id, config, listener) {
  // Create sockets_ map
  this->sockets_.resize(config.parallelism() + 1, -1);
  this->outgoing_counts_.resize(config.parallelism() + 1, 0);
  // Input/read buffers.
  this->read_query_buffer_ = new unsigned char[this->query_msg_size_];
  this->read_response_buffer_ = new unsigned char[this->response_msg_size_];

  // Setup sockets.
  if (machine_id - 1 > 0) {
    int serverfd = ServerSocket(config, party_id, machine_id);
    AcceptClients(serverfd, &this->sockets_, machine_id - 1);
  }
  for (uint32_t m = machine_id + 1; m <= config.parallelism(); m++) {
    this->sockets_[m] = ClientSocket(config, party_id, machine_id, m);
  }
}

// Listening to queries and responses.
void IntraPartyTCPSocket::ListenQueries(std::vector<uint32_t> counts) {
  // Find total number of queries to read.
  uint32_t total = 0;
  for (uint32_t count : counts) {
    total += count;
  }
  total -= counts.at(0) + counts.at(this->machine_id_);

  // Fill fds array.
  nfds_t nfds = this->config_.parallelism();
  pollfd *fds = new pollfd[nfds];
  FillPoll(fds, this->sockets_, counts, this->machine_id_);

  // Read the queries.
  while (total-- > 0) {
    uint32_t machine_id = PollAndFind(fds, nfds);
    // Update counts.
    if (--counts.at(machine_id) == 0) {
      fds[machine_id - 1].fd = -1;
      fds[machine_id - 1].events = 0;
      fds[machine_id - 1].revents = 0;
    }
    // Read query.
    read(this->sockets_.at(machine_id), this->read_query_buffer_,
         this->query_msg_size_);
    this->listener_->OnReceiveQuery(machine_id, this->read_query_buffer_);
  }

  delete[] fds;

  // Read broadcast!
  unsigned char ready = 0;
  size_t size = sizeof(ready);
  for (uint32_t machine_id = 1; machine_id <= nfds; machine_id++) {
    if (machine_id != this->machine_id_) {
      read(this->sockets_.at(machine_id), &ready, size);
      assert(ready == 1);
      ready = 0;
      this->listener_->OnQueriesReady(machine_id);
    }
  }
}

void IntraPartyTCPSocket::ListenResponses() {
  // Find total number of queries to read.
  uint32_t total = 0;
  for (uint32_t count : this->outgoing_counts_) {
    total += count;
  }

  // Fill fds array.
  nfds_t nfds = this->config_.parallelism();
  pollfd *fds = new pollfd[nfds];
  FillPoll(fds, this->sockets_, this->outgoing_counts_, this->machine_id_);

  // Read the queries.
  while (total-- > 0) {
    // At least one socket has something to read, find it!
    uint32_t machine_id = PollAndFind(fds, nfds);
    // Update counts.
    if (--this->outgoing_counts_.at(machine_id) == 0) {
      fds[machine_id - 1].fd = -1;
      fds[machine_id - 1].events = 0;
      fds[machine_id - 1].revents = 0;
    }
    // Read response.
    read(this->sockets_.at(machine_id), this->read_response_buffer_,
         this->response_msg_size_);
    this->listener_->OnReceiveResponse(
        machine_id, types::Response::Deserialize(this->read_response_buffer_));
  }

  delete[] fds;

  // Read broadcast!
  unsigned char ready = 0;
  size_t size = sizeof(ready);
  for (uint32_t machine_id = 1; machine_id <= nfds; machine_id++) {
    if (machine_id != this->machine_id_) {
      read(this->sockets_.at(machine_id), &ready, size);
      assert(ready == 1);
      ready = 0;
      this->listener_->OnResponsesReady(machine_id);
    }
  }
}

// Broadcasts.
void IntraPartyTCPSocket::BroadcastQueriesReady() {
  unsigned char msg = 1;
  for (int socket : this->sockets_) {
    if (socket > -1) {
      send(socket, &msg, sizeof(msg), 0);
    }
  }
  this->listener_->OnQueriesReady(this->machine_id_);
}
void IntraPartyTCPSocket::BroadcastResponsesReady() {
  unsigned char msg = 1;
  for (int socket : this->sockets_) {
    if (socket > -1) {
      send(socket, &msg, sizeof(msg), 0);
    }
  }
  this->listener_->OnResponsesReady(this->machine_id_);
}

// Sending queries and responses.
void IntraPartyTCPSocket::SendQuery(uint32_t machine_id,
                                    const types::OutgoingQuery &query) {
  types::ForwardQuery serialized = query.Serialize();
  if (machine_id == this->machine_id_) {
    this->listener_->OnReceiveQuery(machine_id, serialized);
  } else {
    this->outgoing_counts_.at(machine_id)++;
    send(this->sockets_.at(machine_id), serialized, this->query_msg_size_, 0);
  }
}
void IntraPartyTCPSocket::SendResponse(uint32_t machine_id,
                                       const types::ForwardResponse &response) {
  if (machine_id == this->machine_id_) {
    types::Response deserialized = types::Response::Deserialize(response);
    this->listener_->OnReceiveResponse(machine_id, deserialized);
  } else {
    send(this->sockets_.at(machine_id), response, this->response_msg_size_, 0);
  }
}

// Flushing write buffers.
void IntraPartyTCPSocket::FlushQueries() {}
void IntraPartyTCPSocket::FlushResponses() {}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
