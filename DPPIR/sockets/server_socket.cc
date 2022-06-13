#include "DPPIR/sockets/server_socket.h"

#include <iostream>

#include "DPPIR/sockets/common.h"

namespace DPPIR {
namespace sockets {

// Listen on port and accept connection.
void ServerSocket::Initialize(int port) {
  std::cout << "Creating server and accepting connections..." << std::endl;
  std::cout << "On port " << port << std::endl;
  common::ListenOn(port, &this->sockfd_, 1);
  std::cout << "Client connected!" << std::endl;
}

// Logistics.
index_t ServerSocket::ReadCount() {
  index_t count = 0;
  common::Read(this->sockfd_, reinterpret_cast<char*>(&count), sizeof(count));
  return count;
}
void ServerSocket::SendReady() {
  char ready = 1;
  common::Send(this->sockfd_, &ready, sizeof(ready));
}

// Buffered reads.
CipherLogicalBuffer& ServerSocket::ReadCiphers(index_t read_count) {
  common::Read(this->sockfd_, read_count, &this->cipher_rbuf_);
  return this->cipher_rbuf_;
}
LogicalBuffer<Query>& ServerSocket::ReadQueries(index_t read_count) {
  common::Read(this->sockfd_, read_count, &this->query_rbuf_);
  return this->query_rbuf_;
}

// Buffered send.
void ServerSocket::SendResponse(const Response& response) {
  this->response_wbuf_.PushBack(response);
  if (this->response_wbuf_.Full()) {
    this->FlushResponses();
  }
}

// Buffer flush.
void ServerSocket::FlushResponses() {
  common::Send(this->sockfd_, &this->response_wbuf_);
  this->response_wbuf_.Clear();
}

}  // namespace sockets
}  // namespace DPPIR
