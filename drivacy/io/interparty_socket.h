// Copyright 2020 multiparty.org

// The real server-server socket interface (across different parties).
//
// Every socket connects a client (socket-wise) machine with a server
// (socket-wise) machine. The party with a lower id has the client end
// while the one with the higher id has the server end.

#ifndef DRIVACY_IO_INTERPARTY_SOCKET_H_
#define DRIVACY_IO_INTERPARTY_SOCKET_H_

#include <poll.h>

#include <cstdint>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

class InterPartySocketListener {
 public:
  // Indicates that a batch start message was received specifying the
  // size of the batch.
  virtual void OnReceiveBatchSize(uint32_t batch_size) = 0;
  // Handlers for when a query or response are received.
  virtual void OnReceiveQuery(const types::IncomingQuery &query) = 0;
  virtual void OnReceiveResponse(const types::ForwardResponse &response) = 0;
};

class InterPartyTCPSocket : public AbstractSocket {
 public:
  InterPartyTCPSocket(uint32_t party_id, uint32_t machine_id,
                      const types::Configuration &config,
                      InterPartySocketListener *listener);

  ~InterPartyTCPSocket();

  // AbstractSocket
  uint32_t FdCount() override;
  bool PollQueries(pollfd *fds) override;
  bool PollResponses(pollfd *fds) override;

  bool ReadQuery(uint32_t fd_index, pollfd *fds) override;
  bool ReadResponse(uint32_t fd_index, pollfd *fds) override;

  // New functionality.
  void ReadBatchSize();
  void SendBatchSize(uint32_t batch_size);
  void SendQuery(const types::ForwardQuery &query);
  void SendResponse(const types::Response &response);

 private:
  // Configuration.
  uint32_t party_id_;
  uint32_t machine_id_;
  uint32_t party_count_;
  types::Configuration config_;
  InterPartySocketListener *listener_;
  // Stores the sizes of messages.
  uint32_t incoming_query_msg_size_;
  uint32_t outgoing_query_msg_size_;
  uint32_t response_msg_size_;
  // Socket between this party and the previous one.
  // This party acts as a server.
  int lower_socket_;
  // Socket between this party and the next one.
  // The other party acts as a server.
  int upper_socket_;
  // Number of sent queries (and thus number of responses expected).
  uint32_t queries_count_;
  uint32_t responses_count_;
  // Read buffers for storing a message that was just read from a socket.
  unsigned char *read_query_buffer_;
  unsigned char *read_response_buffer_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_INTERPARTY_SOCKET_H_
