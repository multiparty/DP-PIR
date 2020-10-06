// Copyright 2020 multiparty.org

// The real server-server socket interface.
//
// Every socket connects a client (socket-wise) server with a server
// (socket-wise) server. The party with a lower id has the client end
// while the one with the higher id has the server end.

#ifndef DRIVACY_IO_SOCKET_H_
#define DRIVACY_IO_SOCKET_H_

#define BUFFER_MESSAGE_COUNT 25

#include <cstdint>
#include <memory>
#include <string>
// NOLINTNEXTLINE
#include <thread>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

class TCPSocket : public AbstractSocket {
 public:
  TCPSocket(uint32_t party_id, const types::Configuration &config,
            SocketListener *listener)
      : AbstractSocket(party_id, config, listener) {
    this->queries_written_ = 0;
    this->responses_written_ = 0;
    this->query_buffer_ =
        new unsigned char[BUFFER_MESSAGE_COUNT * this->query_msg_size_];
    this->response_buffer_ =
        new unsigned char[BUFFER_MESSAGE_COUNT * this->response_msg_size_];
  }

  // Free internal write buffers.
  ~TCPSocket() {
    delete this->query_buffer_;
    delete this->response_buffer_;
  }

  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, const types::Configuration &config,
      SocketListener *listener) {
    return std::make_unique<TCPSocket>(party_id, config, listener);
  }

  void SendBatch(uint32_t batch_size) override;
  void SendQuery(const types::OutgoingQuery &query) override;
  void SendResponse(const types::Response &response) override;

  void FlushQueries() override;
  void FlushResponses() override;

  void Listen() override;

 private:
  // Helpers for listening and reading socket buffers.
  void ListenToIncomingQueries();
  void ListenToIncomingResponses();
  // Socket between this party and the previous one.
  // This party acts as a server.
  int lower_socket_;
  // Socket between this party and the next one.
  // The other party acts as a server.
  int upper_socket_;
  // If this class has to listen on both lower and upper sockets,
  // the upper socket listening is done in a separate thread stored here.
  // Otherwise, this thread is empty.
  std::thread thread_;
  // Buffer to store messages being written so that a batch of messages
  // are sent all at once, instead of one at a time.
  unsigned char *query_buffer_;
  unsigned char *response_buffer_;
  // Tracks how many messages have been written to the internal write buffer.
  uint32_t queries_written_;
  uint32_t responses_written_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_SOCKET_H_
