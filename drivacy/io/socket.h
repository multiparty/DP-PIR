// Copyright 2020 multiparty.org

// The real server-server socket interface (across different parties).
//
// Every socket connects a client (socket-wise) machine with a server
// (socket-wise) machine. The party with a lower id has the client end
// while the one with the higher id has the server end.

#ifndef DRIVACY_IO_SOCKET_H_
#define DRIVACY_IO_SOCKET_H_

#include <cstdint>
#include <memory>
#include <string>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

class TCPSocket : public AbstractSocket {
 public:
  TCPSocket(uint32_t party_id, uint32_t machine_id,
            const types::Configuration &config, SocketListener *listener);

  // Free internal write buffers.
  ~TCPSocket() {
    delete[] this->read_query_buffer_;
    delete[] this->read_response_buffer_;
  }

  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, uint32_t machine_id,
      const types::Configuration &config, SocketListener *listener) {
    return std::make_unique<TCPSocket>(party_id, machine_id, config, listener);
  }

  void SendBatch(uint32_t batch_size) override;
  void SendQuery(const types::ForwardQuery &query) override;
  void SendResponse(const types::Response &response) override;

  void Listen() override;

 private:
  // Socket between this party and the previous one.
  // This party acts as a server.
  int lower_socket_;
  // Socket between this party and the next one.
  // The other party acts as a server.
  int upper_socket_;
  // Number of sent queries (and thus number of responses expected).
  uint32_t sent_queries_count_;
  // Read buffers for storing a message that was just read from a socket.
  unsigned char *read_query_buffer_;
  unsigned char *read_response_buffer_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_SOCKET_H_
