// Copyright 2020 multiparty.org

// The real server-server socket interface.
//
// Every socket connects a client (socket-wise) server with a server
// (socket-wise) server. The party with a lower id has the client end
// while the one with the higher id has the server end.

#include "drivacy/io/socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "absl/functional/bind_front.h"

namespace drivacy {
namespace io {
namespace socket {

namespace {

int SocketServer(uint16_t port) {
  // Creating socket file descriptor.
  int serverfd = ::socket(AF_INET, SOCK_STREAM, 0);
  assert(serverfd >= 0);

  // Filling server information.
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;  // IPv4
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(port);

  // Bind the socket to the port address.
  assert(bind(serverfd, reinterpret_cast<struct sockaddr *>(&servaddr),
              sizeof(servaddr)) >= 0);

  // Listen for only 1 connection.
  std::cout << "Waiting for client..." << std::endl;
  assert(listen(serverfd, 1) >= 0);
  std::cout << "Client connected to our server socket!" << std::endl;

  // Accept the connection.
  auto servaddr_len = static_cast<socklen_t>(sizeof(servaddr));
  int sockfd =
      accept(serverfd, reinterpret_cast<sockaddr *>(&servaddr), &servaddr_len);
  assert(sockfd >= 0);
  return sockfd;
}

int SocketClient(uint16_t port, const std::string &serverip) {
  // Creating socket file descriptor.
  int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
  assert(sockfd >= 0);

  // Filling server information.
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;  // IPv4
  servaddr.sin_port = htons(port);
  assert(inet_pton(AF_INET, serverip.c_str(), &servaddr.sin_addr) >= 0);

  // Connect to the server.
  std::cout << "Connecting to server..." << std::endl;
  while (connect(sockfd, reinterpret_cast<sockaddr *>(&servaddr),
                 sizeof(servaddr)) < 0) {
    sleep(1);
  }
  std::cout << "Connected!" << std::endl;

  return sockfd;
}

}  // namespace

// Socket(s) setup ...
void TCPSocket::Listen() {
  // Read ports from config.
  int32_t this_port = this->config_.network().at(this->party_id_).socket_port();
  int32_t next_port = -1;
  std::string next_ip;
  if (this->party_id_ < this->config_.parties()) {
    next_port = this->config_.network().at(this->party_id_ + 1).socket_port();
    next_ip = this->config_.network().at(this->party_id_ + 1).ip();
  }

  // Create socket client to the next party (for responses).
  if (next_port != -1) {
    this->upper_socket_ = SocketClient(next_port, next_ip);
    this->thread_ = std::thread(
        absl::bind_front(&TCPSocket::ListenToIncomingResponses, this));
  }

  // Create socket server to the previous party (for queries).
  if (this_port != -1) {
    this->lower_socket_ = SocketServer(this_port);
    this->ListenToIncomingQueries();
  }
}

// Reading messages ...
void TCPSocket::ListenToIncomingQueries() {
  uint32_t batch_size;
  uint32_t buffer_size =
      types::IncomingQuery::Size(this->party_id_, this->party_count_);
  unsigned char *buffer = new unsigned char[buffer_size];
  while (true) {
    // First, listen to a setup message that defines the size of the batch.
    read(this->lower_socket_, reinterpret_cast<unsigned char *>(&batch_size),
         sizeof(uint32_t));
    this->listener_->OnReceiveBatch(batch_size);

    // Then, expect to read that many queries.
    for (uint32_t i = 0; i < batch_size; i++) {
      read(this->lower_socket_, buffer, buffer_size);
      this->listener_->OnReceiveQuery(
          types::IncomingQuery::Deserialize(buffer, buffer_size));
    }
  }
}

void TCPSocket::ListenToIncomingResponses() {
  uint32_t buffer_size = types::Response::Size();
  unsigned char *buffer = new unsigned char[buffer_size];
  while (true) {
    read(this->upper_socket_, buffer, buffer_size);
    this->listener_->OnReceiveResponse(types::Response::Deserialize(buffer));
  }
}

// Sending messages ...
void TCPSocket::SendBatch(uint32_t batch_size) {
  unsigned char *buffer = reinterpret_cast<unsigned char *>(&batch_size);
  send(this->upper_socket_, buffer, sizeof(uint32_t), 0);
}

void TCPSocket::SendQuery(const types::OutgoingQuery &query) {
  auto [buffer, size] = query.Serialize();
  send(this->upper_socket_, buffer, size, 0);
}

void TCPSocket::SendResponse(const types::Response &response) {
  auto [buffer, size] = response.Serialize();
  send(this->lower_socket_, buffer, size, 0);
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
