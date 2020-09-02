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

namespace drivacy {
namespace io {
namespace socket {

class ServerSideSocket : public AbstractSocket {
 public:
  ServerSideSocket(uint32_t party_id, QueryListener query_listener,
                   ResponseListener response_listener)
      : party_id_(party_id),
        query_listener_(query_listener),
        response_listener_(response_listener) {}

  void SendQuery(uint32_t client, const types::Query &query) const override;
  void SendResponse(uint32_t client_id,
                    const types::Response &response) const override;

  void Listen(const types::Configuration &config) override;

 private:
  uint32_t party_id_;
  QueryListener query_listener_;
  ResponseListener response_listener_;

  void HandleQuery(std::string message, uint32_t client_id) const;
};

class ClientSideSocket : public AbstractSocket {
 public:
  ClientSideSocket(uint32_t party_id, QueryListener query_listener,
                   ResponseListener response_listener)
      : party_id_(party_id),
        query_listener_(query_listener),
        response_listener_(response_listener) {}

  void SendQuery(uint32_t client, const types::Query &query) const override;
  void SendResponse(uint32_t client_id,
                    const types::Response &response) const override;

  void Listen(const types::Configuration &config) override;

 private:
  uint32_t party_id_;
  QueryListener query_listener_;
  ResponseListener response_listener_;

  void HandleQuery(std::string message, uint32_t client_id) const;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_SOCKET_H_
