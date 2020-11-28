// Copyright 2020 multiparty.org

// A unified listener allows listening to several sockets at the same
// time (via poll), so that the internal TCP read buffer of any socket
// is not filled up while our code listens to a different socket.

#include "drivacy/io/unified_listener.h"

#include <cassert>

namespace drivacy {
namespace io {
namespace listener {

// Add this socket to the set of sockets we are listening over.
void UnifiedListener::AddSocket(socket::AbstractSocket *socket) {
  if (socket == nullptr) {
    return;
  }

  uint32_t nfds = socket->FdCount();
  this->sockets_.push_back(socket);
  this->index_to_nfds_.resize(this->nfds_ + nfds, this->nfds_);
  this->nfds_ += nfds;
  this->index_to_socket_.resize(this->nfds_, socket);
  if (this->fds_ != nullptr) {
    delete[] this->fds_;
  }
  this->fds_ = new pollfd[this->nfds_];
}

// Listen to either queries or responses until are underlying sockets are
// consumed.
void UnifiedListener::ListenToNoiseQueries() {
  // Initialze fds.
  socket::AbstractSocket *socket = this->sockets_.back();
  uint32_t fd_count = socket->FdCount();
  uint32_t offset = this->nfds_ - fd_count;
  bool consumed_socket = socket->PollNoiseQueries(this->fds_ + offset);
  // Poll and read until socket is consumed.
  while (!consumed_socket) {
    // Poll for some socket.
    assert(poll(this->fds_ + offset, fd_count, -1) > 0);
    // Find all sockets that are ready to read.
    for (uint32_t i = 0; i < fd_count; i++) {
      if (this->fds_[offset + i].revents & POLLIN) {
        consumed_socket = socket->ReadNoiseQuery(i, this->fds_ + offset);
      }
    }
  }
}
void UnifiedListener::ListenToQueries() {
  uint32_t consumed_sockets = 0;
  // Initialze fds.
  uint32_t offset = 0;
  for (socket::AbstractSocket *socket : this->sockets_) {
    if (socket->PollQueries(this->fds_ + offset)) {
      consumed_sockets++;
    }
    offset += socket->FdCount();
  }
  // Poll and read until all sockets are consumed.
  while (consumed_sockets < this->sockets_.size()) {
    // Poll for some socket.
    assert(poll(this->fds_, this->nfds_, -1) > 0);
    // Find all sockets that are ready to read.
    for (uint32_t i = 0; i < this->nfds_; i++) {
      if (this->fds_[i].revents & POLLIN) {
        socket::AbstractSocket *socket = this->index_to_socket_[i];
        uint32_t offset = this->index_to_nfds_[i];
        if (socket->ReadQuery(i - offset, this->fds_ + offset)) {
          consumed_sockets++;
        }
      }
    }
  }
}
void UnifiedListener::ListenToResponses() {
  uint32_t consumed_sockets = 0;
  // Initialze fds.
  uint32_t offset = 0;
  for (socket::AbstractSocket *socket : this->sockets_) {
    if (socket->PollResponses(this->fds_ + offset)) {
      consumed_sockets++;
    }
    offset += socket->FdCount();
  }
  // Poll and read until all sockets are consumed.
  while (consumed_sockets < this->sockets_.size()) {
    // Poll for some socket.
    assert(poll(this->fds_, this->nfds_, -1) > 0);
    // Find all sockets that are ready to read.
    for (uint32_t i = 0; i < this->nfds_; i++) {
      if (this->fds_[i].revents & POLLIN) {
        socket::AbstractSocket *socket = this->index_to_socket_[i];
        uint32_t offset = this->index_to_nfds_[i];
        if (socket->ReadResponse(i - offset, this->fds_ + offset)) {
          consumed_sockets++;
        }
      }
    }
  }
}

// Nonblocking versions.
void UnifiedListener::ListenToNoiseQueriesNonblocking() {
  // Initialze fds.
  socket::AbstractSocket *socket = this->sockets_.back();
  uint32_t fd_count = socket->FdCount();
  uint32_t offset = this->nfds_ - fd_count;
  socket->PollNoiseQueries(this->fds_ + offset);
  // Poll and read until all sockets are consumed.
  while (poll(this->fds_ + offset, fd_count, 0) > 0) {
    // Find all sockets that are ready to read.
    for (uint32_t i = 0; i < fd_count; i++) {
      if (this->fds_[offset + i].revents & POLLIN) {
        socket->ReadNoiseQuery(i, this->fds_ + offset);
      }
    }
  }
}
void UnifiedListener::ListenToQueriesNonblocking() {
  // Initialze fds.
  uint32_t offset = 0;
  for (socket::AbstractSocket *socket : this->sockets_) {
    socket->PollQueries(this->fds_ + offset);
    offset += socket->FdCount();
  }
  // Poll and read until all sockets are consumed.
  while (poll(this->fds_, this->nfds_, 0) > 0) {
    // Find all sockets that are ready to read.
    for (uint32_t i = 0; i < this->nfds_; i++) {
      if (this->fds_[i].revents & POLLIN) {
        socket::AbstractSocket *socket = this->index_to_socket_[i];
        uint32_t offset = this->index_to_nfds_[i];
        socket->ReadQuery(i - offset, this->fds_ + offset);
      }
    }
  }
}

}  // namespace listener
}  // namespace io
}  // namespace drivacy
