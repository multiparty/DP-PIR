// Copyright 2020 multiparty.org

// The real server-server socket interface (across different parties).
//
// Every socket connects a client (socket-wise) machine with a server
// (socket-wise) machine. The party with a lower id has the client end
// while the one with the higher id has the server end.

#include "drivacy/io/interparty_socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

#include "drivacy/primitives/crypto.h"

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

int SocketServer(uint16_t port) {
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

  // Listen for only 1 connection.
  std::cout << "Waiting for client..." << std::endl;
  assert(listen(serverfd, 1) >= 0);

  // Accept the connection.
  auto servaddr_len = static_cast<socklen_t>(sizeof(servaddr));
  int sockfd =
      accept(serverfd, reinterpret_cast<sockaddr *>(&servaddr), &servaddr_len);
  assert(sockfd >= 0);
  std::cout << "Client connected to our server socket!" << std::endl;
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
InterPartyTCPSocket::InterPartyTCPSocket(uint32_t party_id, uint32_t machine_id,
                                         const types::Configuration &config,
                                         InterPartySocketListener *listener)
    : party_id_(party_id),
      machine_id_(machine_id),
      config_(config),
      listener_(listener) {
  // Initialize configurations.
  this->party_count_ = config.parties();

  // Initialize the various message sizes.
  this->incoming_message_size_ =
      primitives::crypto::OnionCipherSize(party_id, this->party_count_);
  this->outgoing_message_size_ =
      primitives::crypto::OnionCipherSize(party_id + 1, this->party_count_);

  // Default counter values.
  this->messages_count_ = 0;
  this->queries_count_ = 0;
  this->responses_count_ = 0;

  // Default no socket marker.
  this->lower_socket_ = -1;
  this->upper_socket_ = -1;

  // Input/read buffers.
  this->read_message_buffer_ = new unsigned char[this->incoming_message_size_];

  // Read ports from config.
  const auto &network = this->config_.network();
  const auto &this_party_network = network.at(this->party_id_).machines();
  int32_t this_port = this_party_network.at(machine_id).socket_port();
  int32_t next_port = -1;
  std::string next_ip;
  if (this->party_id_ < this->config_.parties()) {
    const auto &next_party_network = network.at(this->party_id_ + 1).machines();
    next_port = next_party_network.at(machine_id).socket_port();
    next_ip = next_party_network.at(machine_id).ip();
  }

  // Create socket server to the previous party (for queries).
  if (this_port != -1) {
    this->lower_socket_ = SocketServer(this_port);
  }

  // Create socket client to the next party (for responses).
  if (next_port != -1) {
    this->upper_socket_ = SocketClient(next_port, next_ip);
  }
}

InterPartyTCPSocket::~InterPartyTCPSocket() {
  if (this->lower_socket_ > -1) {
    close(this->lower_socket_);
  }
  if (this->upper_socket_ > -1) {
    close(this->upper_socket_);
  }
  delete[] this->read_message_buffer_;
}

// Configure poll(...).
uint32_t InterPartyTCPSocket::FdCount() {
  // We have 2 fds but they are completely disjoint.
  return 1;
}

bool InterPartyTCPSocket::PollMessages(pollfd *fds) {
  fds->revents = 0;
  if (this->messages_count_ > 0 && this->lower_socket_ != -1) {
    fds->fd = this->lower_socket_;
    fds->events = POLLIN;
    return false;
  } else {
    fds->fd = -1;
    fds->events = 0;
    return true;
  }
}
bool InterPartyTCPSocket::PollQueries(pollfd *fds) {
  fds->revents = 0;
  if (this->queries_count_ > 0) {
    fds->fd = this->lower_socket_;
    fds->events = POLLIN;
    return false;
  } else {
    fds->fd = -1;
    fds->events = 0;
    return true;
  }
}

bool InterPartyTCPSocket::PollResponses(pollfd *fds) {
  fds->revents = 0;
  if (this->responses_count_ > 0) {
    fds->fd = this->upper_socket_;
    fds->events = POLLIN;
    return false;
  } else {
    fds->fd = -1;
    fds->events = 0;
    return true;
  }
}

// Reading messages!
void InterPartyTCPSocket::ReadBatchSize() {
  if (this->lower_socket_ != -1) {
    ReadUntil(this->lower_socket_,
              reinterpret_cast<unsigned char *>(&this->queries_count_),
              sizeof(uint32_t));
    this->messages_count_ = this->queries_count_;
    this->listener_->OnReceiveBatchSize(this->queries_count_);
  }
}

bool InterPartyTCPSocket::ReadMessage(uint32_t fd_index, pollfd *fds) {
  assert(fd_index == 0 && this->lower_socket_ != -1 &&
         this->messages_count_ > 0);
  // Blocking read.
  ReadUntil(this->lower_socket_, this->read_message_buffer_,
            this->incoming_message_size_);

  // Chek if done and update fds.
  bool done = (--this->messages_count_ == 0);
  if (done) {
    fds->fd = -1;
    fds->events = 0;
  }
  fds->revents = 0;

  // Call event handler.
  this->listener_->OnReceiveMessage(this->read_message_buffer_);
  return done;
}

bool InterPartyTCPSocket::ReadQuery(uint32_t fd_index, pollfd *fds) {
  assert(fd_index == 0 && this->lower_socket_ != -1 &&
         this->queries_count_ > 0);
  // Blocking read.
  ReadUntil(this->lower_socket_,
            reinterpret_cast<unsigned char *>(&this->read_query_buffer_),
            sizeof(types::Query));

  // Chek if done and update fds.
  bool done = (--this->queries_count_ == 0);
  if (done) {
    fds->fd = -1;
    fds->events = 0;
  }
  fds->revents = 0;

  // Call event handler.
  this->listener_->OnReceiveQuery(this->read_query_buffer_);
  return done;
}

bool InterPartyTCPSocket::ReadResponse(uint32_t fd_index, pollfd *fds) {
  assert(fd_index == 0 && this->upper_socket_ != -1 &&
         this->responses_count_ > 0);
  // Blocking read.
  ReadUntil(this->upper_socket_,
            reinterpret_cast<unsigned char *>(&this->read_response_buffer_),
            sizeof(types::Response));

  // Chek if done and update fds.
  bool done = (--this->responses_count_ == 0);
  if (done) {
    fds->fd = -1;
    fds->events = 0;
  }
  fds->revents = 0;

  // Call event handler.
  this->listener_->OnReceiveResponse(this->read_response_buffer_);
  return done;
}

// Sending messages!
void InterPartyTCPSocket::SendBatchSize(uint32_t batch_size) {
  unsigned char *buffer = reinterpret_cast<unsigned char *>(&batch_size);
  SendAssert(this->upper_socket_, buffer, sizeof(uint32_t));
}

void InterPartyTCPSocket::SendMessage(const types::CipherText &message) {
  SendAssert(this->upper_socket_, message, this->outgoing_message_size_);
}

void InterPartyTCPSocket::SendQuery(const types::Query &query) {
  // Write message to out buffer.
  this->responses_count_++;
  SendAssert(this->upper_socket_,
             reinterpret_cast<const unsigned char *>(&query),
             sizeof(types::Query));
}

void InterPartyTCPSocket::SendResponse(const types::Response &response) {
  // Write message to out buffer.
  SendAssert(this->lower_socket_,
             reinterpret_cast<const unsigned char *>(&response),
             sizeof(types::Response));
}

// Only for timing and benchmarking...
void InterPartyTCPSocket::SendDone() {
  if (this->lower_socket_ != -1) {
    unsigned char done = 1;
    SendAssert(this->lower_socket_,
               reinterpret_cast<const unsigned char *>(&done), sizeof(done));
  }
}
void InterPartyTCPSocket::WaitForDone() {
  if (this->upper_socket_ != -1) {
    unsigned char done;
    ReadUntil(this->upper_socket_, reinterpret_cast<unsigned char *>(&done),
              sizeof(done));
    assert(done == 1);
  }
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
