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

#include <cstring>
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
TCPSocket::TCPSocket(uint32_t party_id, const types::Configuration &config,
                     SocketListener *listener)
    : AbstractSocket(party_id, config, listener) {
  // Default counter values.
  this->queries_written_ = 0;
  this->responses_written_ = 0;
  this->sent_queries_count_ = 0;

  // Default no socket marker.
  this->lower_socket_ = -1;
  this->upper_socket_ = -1;

  // Output/write buffers.
  this->write_query_buffer_ =
      new unsigned char[BUFFER_MESSAGE_COUNT * this->outgoing_query_msg_size_];
  this->write_response_buffer_ =
      new unsigned char[BUFFER_MESSAGE_COUNT * this->response_msg_size_];

  // Input/read buffers.
  this->read_query_buffer_ = new unsigned char[this->incoming_query_msg_size_];
  this->read_response_buffer_ = new unsigned char[this->response_msg_size_];

  // Read ports from config.
  int32_t this_port = this->config_.network().at(this->party_id_).socket_port();
  int32_t next_port = -1;
  std::string next_ip;
  if (this->party_id_ < this->config_.parties()) {
    next_port = this->config_.network().at(this->party_id_ + 1).socket_port();
    next_ip = this->config_.network().at(this->party_id_ + 1).ip();
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

// Reading messages ...
void TCPSocket::Listen() {
  while (true) {
    if (this->lower_socket_ != -1) {
      // First, listen to a setup message that defines the size of the batch.
      uint32_t batch_size;
      read(this->lower_socket_, reinterpret_cast<unsigned char *>(&batch_size),
           sizeof(uint32_t));
      this->listener_->OnReceiveBatch(batch_size);

      // Then, expect to read that many queries.
      for (uint32_t i = 0; i < batch_size; i++) {
        read(this->lower_socket_, this->read_query_buffer_,
             this->incoming_query_msg_size_);
        this->listener_->OnReceiveQuery(types::IncomingQuery::Deserialize(
            this->read_query_buffer_, this->incoming_query_msg_size_));
      }
    }

    if (this->upper_socket_ != -1) {
      // Read however many responses as queries sent.
      // "this->sent_queries_count_" is updated in "SendQuery"
      // it is larger than the batch size, because on top
      // of sending all the received queries, the protocol requires
      // additional noise queries be sent as well.
      for (uint32_t i = 0; i < this->sent_queries_count_; i++) {
        read(this->upper_socket_, this->read_response_buffer_,
             this->response_msg_size_);
        this->listener_->OnReceiveResponse(
            types::Response::Deserialize(this->read_response_buffer_));
      }
    }

    // Reset the number of queries sent (this batch is done).
    this->sent_queries_count_ = 0;

    // If the queries are received on a different socket, return so
    // calling code can listen on that socket.
    if (this->lower_socket_ == -1) {
      break;
    }
  }
}

// Sending messages ...
void TCPSocket::SendBatch(uint32_t batch_size) {
  unsigned char *buffer = reinterpret_cast<unsigned char *>(&batch_size);
  send(this->upper_socket_, buffer, sizeof(uint32_t), 0);
}

void TCPSocket::SendQuery(const types::OutgoingQuery &query) {
  // Write message to out buffer.
  this->sent_queries_count_++;
  const unsigned char *buffer = query.Serialize();
  uint32_t offset = this->queries_written_ * this->outgoing_query_msg_size_;
  memcpy(this->write_query_buffer_ + offset, buffer,
         this->outgoing_query_msg_size_);
  this->queries_written_++;
  // Flush out buffer when full.
  if (this->queries_written_ == BUFFER_MESSAGE_COUNT) {
    this->FlushQueries();
  }
}

void TCPSocket::FlushQueries() {
  if (this->queries_written_ > 0) {
    send(this->upper_socket_, this->write_query_buffer_,
         this->queries_written_ * this->outgoing_query_msg_size_, 0);
    this->queries_written_ = 0;
  }
}

void TCPSocket::SendResponse(const types::Response &response) {
  // Write message to out buffer.
  const unsigned char *buffer = response.Serialize();
  uint32_t offset = this->responses_written_ * this->response_msg_size_;
  memcpy(this->write_response_buffer_ + offset, buffer,
         this->response_msg_size_);
  this->responses_written_++;
  // Flush out buffer when full.
  if (this->responses_written_ == BUFFER_MESSAGE_COUNT) {
    this->FlushResponses();
  }
}

void TCPSocket::FlushResponses() {
  if (this->responses_written_ > 0) {
    send(this->lower_socket_, this->write_response_buffer_,
         this->responses_written_ * this->response_msg_size_, 0);
    this->responses_written_ = 0;
  }
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
