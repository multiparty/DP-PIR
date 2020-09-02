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

#include <istream>

namespace drivacy {
namespace io {
namespace socket {

namespace {

void SocketSend(uint32_t type, const std::string &msg, uint32_t port) {
  int sockfd;
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));

  // Creating socket file descriptor
  sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
  assert(sockfd >= 0);

  // Filling server information
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  servaddr.sin_addr.s_addr = INADDR_ANY;

  // Send size+type prefix first.
  uint32_t size = msg.size();
  size += (type << 31);
  sendto(sockfd, reinterpret_cast<char *>(&size), 4, MSG_CONFIRM,
         (const struct sockaddr *)&servaddr, sizeof(servaddr));

  // Send the message.
  sendto(sockfd, msg.c_str(), msg.size(), MSG_CONFIRM,
         (const struct sockaddr *)&servaddr, sizeof(servaddr));
}

int SocketBind(int32_t port) {
  // Store addresses.
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));

  // Creating socket file descriptor.
  int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
  assert(sockfd >= 0);

  // Filling server information.
  servaddr.sin_family = AF_INET;  // IPv4
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(port);

  // Bind the socket with the server address.
  assert(bind(sockfd, reinterpret_cast<sockaddr *>(&servaddr),
              sizeof(servaddr)) >= 0);
  return sockfd;
}

}  // namespace

// UDPSocket.
void UDPSocket::Listen(const types::Configuration &config) {
  // Read config.
  this->config_ = config;
  int32_t port = config.network().at(this->party_id_).socket_port();

  // Create socket stream on bound socket.
  this->socket_iterator_.Initialize(SocketBind(port));
  while (this->socket_iterator_.HasNext()) {
    const auto [buffer, size, type] = this->socket_iterator_.Next();
    std::string msg(buffer, size);
    if (type == QUERY_MSG_TYPE) {
      types::Query query;
      query.ParseFromString(msg);
      this->query_listener_(this->party_id_ - 1, query);
      continue;
    }
    if (type == RESPONSE_MSG_TYPE) {
      types::Response response;
      response.ParseFromString(msg);
      this->response_listener_(this->party_id_ + 1, response);
      continue;
    }
    assert(false);
  }

  assert(!this->on_);
}

void UDPSocket::SendQuery(uint32_t party_id, const types::Query &query) const {
  int32_t port = this->config_.network().at(party_id).socket_port();
  std::string serialized;
  assert(query.SerializeToString(&serialized));
  SocketSend(QUERY_MSG_TYPE, serialized, port);
}

void UDPSocket::SendResponse(uint32_t party_id,
                             const types::Response &response) const {
  int32_t port = this->config_.network().at(party_id).socket_port();
  std::string serialized;
  assert(response.SerializeToString(&serialized));
  SocketSend(RESPONSE_MSG_TYPE, serialized, port);
}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
