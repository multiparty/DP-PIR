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

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/messages.pb.h"
#include "drivacy/util/socket_iterator.h"

#define QUERY_MSG_TYPE 0
#define RESPONSE_MSG_TYPE 1

namespace drivacy {
namespace io {
namespace socket {

class UDPSocket : public AbstractSocket {
 public:
  UDPSocket(uint32_t party_id, QueryListener query_listener,
            ResponseListener response_listener)
      : party_id_(party_id),
        query_listener_(query_listener),
        response_listener_(response_listener) {}

  void SendQuery(uint32_t party_id, const types::Query &query) const override;
  void SendResponse(uint32_t party_id,
                    const types::Response &response) const override;

  void Listen(const types::Configuration &config) override;
  void Close() override {
    this->on_ = false;
    this->socket_iterator_.Close();
  }

 private:
  uint32_t party_id_;
  QueryListener query_listener_;
  ResponseListener response_listener_;
  bool on_;
  types::Configuration config_;
  util::SocketIterator socket_iterator_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_SOCKET_H_
