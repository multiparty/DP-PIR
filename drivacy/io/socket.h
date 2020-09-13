// Copyright 2020 multiparty.org

// The real server-server socket interface.
//
// Every socket connects a client (socket-wise) server with a server
// (socket-wise) server. The party with a lower id has the client end
// while the one with the higher id has the server end.

#ifndef DRIVACY_IO_SOCKET_H_
#define DRIVACY_IO_SOCKET_H_

#include <cstdint>
#include <string>
// NOLINTNEXTLINE
#include <thread>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

#define QUERY_MSG_TYPE 0
#define RESPONSE_MSG_TYPE 1

namespace drivacy {
namespace io {
namespace socket {

class UDPSocket : public AbstractSocket {
 public:
  UDPSocket(uint32_t party_id, QueryListener query_listener,
            ResponseListener response_listener,
            const types::Configuration &config)
      : AbstractSocket(party_id, query_listener, response_listener, config) {}

  void SendQuery(const types::OutgoingQuery &query) override;
  void SendResponse(const types::Response &response) override;

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
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_SOCKET_H_
