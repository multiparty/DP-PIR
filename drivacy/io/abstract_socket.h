// Copyright 2020 multiparty.org

// Defines the socket API used by implementations for simulated local
// environemnts, and real over-the-wire deployments.

#ifndef DRIVACY_IO_ABSTRACT_SOCKET_H_
#define DRIVACY_IO_ABSTRACT_SOCKET_H_

#include <poll.h>

#include <cstdint>

namespace drivacy {
namespace io {
namespace socket {

// An AbstractSocket is something we can poll and read.
class AbstractSocket {
 public:
  // The count of the underlying file descriptors.
  virtual uint32_t FdCount() = 0;
  // Fill fds[0 ... FdCount()].
  virtual bool PollNoiseMessages(pollfd *fds) = 0;
  virtual bool PollMessages(pollfd *fds) = 0;
  virtual bool PollNoiseQueries(pollfd *fds) = 0;
  virtual bool PollQueries(pollfd *fds) = 0;
  virtual bool PollResponses(pollfd *fds) = 0;
  // Read a query or a response.
  // In addition to reading from fd[fd_index] and calling the appropriate
  // event handler, this function modifies fds[fd_index] depending on whether
  // or not more messages are expected.
  // Returns false when all sockets are completely consumed.
  virtual bool ReadNoiseMessage(uint32_t fd_index, pollfd *fds) = 0;
  virtual bool ReadMessage(uint32_t fd_index, pollfd *fds) = 0;
  virtual bool ReadNoiseQuery(uint32_t fd_index, pollfd *fds) = 0;
  virtual bool ReadQuery(uint32_t fd_index, pollfd *fds) = 0;
  virtual bool ReadResponse(uint32_t fd_index, pollfd *fds) = 0;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_ABSTRACT_SOCKET_H_
