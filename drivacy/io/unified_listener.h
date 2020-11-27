// Copyright 2020 multiparty.org

// A unified listener allows listening to several sockets at the same
// time (via poll), so that the internal TCP read buffer of any socket
// is not filled up while our code listens to a different socket.

#ifndef DRIVACY_IO_UNIFIED_LISTENER_H_
#define DRIVACY_IO_UNIFIED_LISTENER_H_

#include <poll.h>

#include <vector>

#include "drivacy/io/abstract_socket.h"

namespace drivacy {
namespace io {
namespace listener {

class UnifiedListener {
 public:
  // Constructor.
  UnifiedListener() : nfds_(0), fds_(nullptr) {}

  // Not copyable or movable.
  UnifiedListener(UnifiedListener &&other) = delete;
  UnifiedListener &operator=(UnifiedListener &&other) = delete;
  UnifiedListener(const UnifiedListener &) = delete;
  UnifiedListener &operator=(const UnifiedListener &) = delete;

  // Destructor.
  ~UnifiedListener() {
    if (this->fds_ != nullptr) {
      delete[] this->fds_;
    }
  }

  // Add this socket to the set of sockets we are listening over.
  void AddSocket(socket::AbstractSocket *socket);

  // Listen to either queries or responses until are underlying sockets are
  // consumed.
  void ListenToQueries();
  void ListenToResponses();

  void ListenToQueriesNonblocking();
  void ListenToResponsesNonblocking();

 protected:
  // All underlying sockets in a vector.
  std::vector<socket::AbstractSocket *> sockets_;
  // Map an fd index to the abstract socket owning that fd and count of fds
  // owned by that socket.
  std::vector<socket::AbstractSocket *> index_to_socket_;
  std::vector<uint32_t> index_to_nfds_;
  // Poll information.
  nfds_t nfds_;
  pollfd *fds_;
};

}  // namespace listener
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_UNIFIED_LISTENER_H_
