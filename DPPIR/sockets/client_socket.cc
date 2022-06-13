#include "DPPIR/sockets/client_socket.h"

#include <cassert>
#include <iostream>

#include "DPPIR/sockets/common.h"

namespace DPPIR {
namespace sockets {

// Connect to server.
void ClientSocket::Initialize(const std::string& ip, int port) {
  std::cout << "Connecting to server..." << std::endl;
  std::cout << "Server at " << ip << ":" << port << std::endl;
  this->sockfd_ = common::ConnectTo(ip.c_str(), port);
  std::cout << "Connected to the server!" << std::endl;
}

// Logistics.
void ClientSocket::SendCount(index_t count) {
  common::Send(this->sockfd_, reinterpret_cast<char*>(&count), sizeof(count));
}
void ClientSocket::WaitForReady() {
  char ready = 0;
  common::Read(this->sockfd_, &ready, sizeof(char));
  assert(ready == 1);
}

// Buffered send.
void ClientSocket::SendCipher(const char* onion_cipher) {
  this->cipher_wbuf_.PushBack(onion_cipher);
  if (this->cipher_wbuf_.Full()) {
    this->FlushCiphers();
  }
}
void ClientSocket::SendQuery(const Query& query) {
  this->query_wbuf_.PushBack(query);
  if (this->query_wbuf_.Full()) {
    this->FlushQueries();
  }
}

// Buffer flush.
void ClientSocket::FlushCiphers() {
  common::Send(this->sockfd_, &this->cipher_wbuf_);
  this->cipher_wbuf_.Clear();
}
void ClientSocket::FlushQueries() {
  common::Send(this->sockfd_, &this->query_wbuf_);
  this->query_wbuf_.Clear();
}

// Read responses.
LogicalBuffer<Response>& ClientSocket::ReadResponses(index_t read_count) {
  common::Read(this->sockfd_, read_count, &this->response_rbuf_);
  return this->response_rbuf_;
}

}  // namespace sockets
}  // namespace DPPIR
