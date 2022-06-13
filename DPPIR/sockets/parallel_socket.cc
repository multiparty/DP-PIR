#include "DPPIR/sockets/parallel_socket.h"

#include <cassert>
#include <iostream>
#include <memory>

#include "DPPIR/sockets/common.h"

namespace DPPIR {
namespace sockets {

// Constructor.
ParallelSocket::ParallelSocket(server_id_t server_id, server_id_t server_count,
                               size_t cipher_size)
    : server_id_(server_id),
      server_count_(server_count),
      // Initializes each element using default constructor.
      sockfds_(server_id, server_count),
      pollfds_(server_id, server_count),
      // Physical buffers.
      rbuffers_(server_id, server_count),
      wbuffers_(server_id, server_count),
      // Buffers
      cipher_rbufs_(server_id, server_count),
      cipher_wbufs_(server_id, server_count),
      secret_rbufs_(server_id, server_count),
      secret_wbufs_(server_id, server_count),
      query_rbufs_(server_id, server_count),
      query_wbufs_(server_id, server_count),
      response_rbufs_(server_id, server_count),
      response_wbufs_(server_id, server_count) {
  // Initialize logical buffers inside ServersMaps.
  for (server_id_t id = 0; id < server_count; id++) {
    if (id != server_id) {
      // Get the physical buffers.
      auto& rbuf = this->rbuffers_[id];
      auto& wbuf = this->wbuffers_[id];
      // Initialize logical buffers using physical ones.
      this->cipher_rbufs_[id] = CipherLogicalBuffer(&rbuf, cipher_size);
      this->cipher_wbufs_[id] = CipherLogicalBuffer(&wbuf, cipher_size);
      this->secret_rbufs_[id] = LogicalBuffer<OfflineSecret>(&rbuf);
      this->secret_wbufs_[id] = LogicalBuffer<OfflineSecret>(&wbuf);
      this->query_rbufs_[id] = LogicalBuffer<Query>(&rbuf);
      this->query_wbufs_[id] = LogicalBuffer<Query>(&wbuf);
      this->response_rbufs_[id] = LogicalBuffer<Response>(&rbuf);
      this->response_wbufs_[id] = LogicalBuffer<Response>(&wbuf);
    }
  }
}

// Create server if need to and connect to all parallel servers.
void ParallelSocket::Initialize(const config::PartyConfig& config) {
  std::cout << "Initializing parallel connections..." << std::endl;

  // We are a server to all servers with id < server_id_.
  if (this->server_id_ > 0) {
    std::cout << "Creating parallel server and accepting connections..."
              << std::endl;
    int port = config.servers[this->server_id_].parallel_port;
    std::cout << "On port " << port << std::endl;
    std::unique_ptr<int[]> fds = std::make_unique<int[]>(this->server_id_);
    common::ListenOn(port, fds.get(), this->server_id_);
    for (server_id_t id = 0; id < this->server_id_; id++) {
      int fd = fds[id];
      // Figure out which server this is.
      server_id_t server_id;
      common::Read(fd, reinterpret_cast<char*>(&server_id), sizeof(server_id));
      assert(server_id < this->server_id_);
      // Fill in relevant pollfd struct.
      this->sockfds_[server_id] = fd;
      pollfd& v = this->pollfds_[server_id];
      v.fd = fd;
      v.events = POLLIN;
      v.revents = 0;
    }
    std::cout << "Parallel clients connected..." << std::endl;
  }

  // We connect to all servers with id > server_id_.
  server_id_t count = this->server_count_ - this->server_id_;
  for (server_id_t i = 1; i < count; i++) {
    server_id_t id = this->server_id_ + i;
    const config::ServerConfig& conf = config.servers[id];
    // Connect to server id.
    std::cout << "Connecting to parallel server " << int(id) << std::endl;
    std::cout << "Server at  " << conf.ip << ":" << conf.parallel_port
              << std::endl;
    int fd = common::ConnectTo(conf.ip.c_str(), conf.parallel_port);
    // Fill in pollfd struct.
    this->sockfds_[id] = fd;
    pollfd& v = this->pollfds_[id];
    v.fd = fd;
    v.events = POLLIN;
    v.revents = 0;
    // Declare identity to server.
    common::Send(fd, reinterpret_cast<char*>(&this->server_id_),
                 sizeof(this->server_id_));
    std::cout << "Connected to parallel server " << int(id) << std::endl;
  }
}

// Logistics.
void ParallelSocket::SendCount(server_id_t target, index_t count) {
  int sockfd = this->sockfds_[target];
  common::Send(sockfd, reinterpret_cast<char*>(&count), sizeof(count));
}
void ParallelSocket::BroadcastCount(index_t count) {
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->SendCount(id, count);
    }
  }
}
index_t ParallelSocket::ReadCount(server_id_t id) {
  index_t count;
  common::Read(this->sockfds_[id], reinterpret_cast<char*>(&count),
               sizeof(count));
  return count;
}
void ParallelSocket::BroadcastReady() {
  char ready = 1;
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      int sockfd = this->sockfds_[id];
      common::Send(sockfd, &ready, sizeof(ready));
    }
  }
}
void ParallelSocket::WaitForReady() {
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      char ready = 0;
      int sockfd = this->sockfds_[id];
      common::Read(sockfd, &ready, sizeof(ready));
      assert(ready == 1);
    }
  }
}

// Poll API.
server_id_t ParallelSocket::Poll(ServersMap<bool>* outs, int timeout) {
  return common::Poll(this->pollfds_.Ptr(), this->server_count_ - 1, timeout,
                      outs->Ptr());
}
void ParallelSocket::IgnoreServer(server_id_t id) {
  this->pollfds_[id].fd = this->sockfds_[id] * -1;
}
void ParallelSocket::ResetServers() {
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->pollfds_[id].fd = this->sockfds_[id];
    }
  }
}

// Reading API.
CipherLogicalBuffer& ParallelSocket::ReadCiphers(server_id_t source,
                                                 index_t read_count) {
  int fd = this->sockfds_[source];
  common::Read(fd, read_count, &this->cipher_rbufs_[source]);
  return this->cipher_rbufs_[source];
}

// This can only be read from sibling servers, not the back socket.
LogicalBuffer<OfflineSecret>& ParallelSocket::ReadSecrets(server_id_t source,
                                                          index_t read_count) {
  int fd = this->sockfds_[source];
  common::Read(fd, read_count, &this->secret_rbufs_[source]);
  return this->secret_rbufs_[source];
}

LogicalBuffer<Query>& ParallelSocket::ReadQueries(server_id_t source,
                                                  index_t read_count) {
  int fd = this->sockfds_[source];
  common::Read(fd, read_count, &this->query_rbufs_[source]);
  return this->query_rbufs_[source];
}

LogicalBuffer<Response>& ParallelSocket::ReadResponses(server_id_t source,
                                                       index_t read_count) {
  int fd = this->sockfds_[source];
  common::Read(fd, read_count, &this->response_rbufs_[source]);
  return this->response_rbufs_[source];
}

// Writing API: same as singular socket but takes the target server_id as arg.
void ParallelSocket::SendCipher(server_id_t target, const char* onion_cipher) {
  CipherLogicalBuffer& buffer = this->cipher_wbufs_[target];
  buffer.PushBack(onion_cipher);
  if (buffer.Full()) {
    this->FlushCiphers(target);
  }
}
void ParallelSocket::SendQuery(server_id_t target, const Query& query) {
  LogicalBuffer<Query>& buffer = this->query_wbufs_[target];
  buffer.PushBack(query);
  if (buffer.Full()) {
    this->FlushQueries(target);
  }
}
void ParallelSocket::SendResponse(server_id_t target, const Response& r) {
  LogicalBuffer<Response>& buffer = this->response_wbufs_[target];
  buffer.PushBack(r);
  if (buffer.Full()) {
    this->FlushResponses(target);
  }
}
void ParallelSocket::BroadcastSecret(const OfflineSecret& secret) {
  for (server_id_t target = 0; target < this->server_count_; target++) {
    if (target != this->server_id_) {
      LogicalBuffer<OfflineSecret>& buffer = this->secret_wbufs_[target];
      buffer.PushBack(secret);
      if (buffer.Full()) {
        this->FlushSecrets(target);
      }
    }
  }
}

// Flush all target server buffers.
void ParallelSocket::FlushCiphers() {
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->FlushCiphers(id);
    }
  }
}
void ParallelSocket::FlushSecrets() {
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->FlushSecrets(id);
    }
  }
}
void ParallelSocket::FlushQueries() {
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->FlushQueries(id);
    }
  }
}
void ParallelSocket::FlushResponses() {
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      this->FlushResponses(id);
    }
  }
}

// Flush buffer of a specific target server.
void ParallelSocket::FlushCiphers(server_id_t id) {
  common::Send(this->sockfds_[id], &this->cipher_wbufs_[id]);
  this->cipher_wbufs_[id].Clear();
}
void ParallelSocket::FlushSecrets(server_id_t id) {
  common::Send(this->sockfds_[id], &this->secret_wbufs_[id]);
  this->secret_wbufs_[id].Clear();
}
void ParallelSocket::FlushQueries(server_id_t id) {
  common::Send(this->sockfds_[id], &this->query_wbufs_[id]);
  this->query_wbufs_[id].Clear();
}
void ParallelSocket::FlushResponses(server_id_t id) {
  common::Send(this->sockfds_[id], &this->response_wbufs_[id]);
  this->response_wbufs_[id].Clear();
}

}  // namespace sockets
}  // namespace DPPIR
