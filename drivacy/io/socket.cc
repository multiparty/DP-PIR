// Copyright 2020 multiparty.org

// The real server-server socket interface.
//
// Every socket connects a client (socket-wise) server with a server
// (socket-wise) server. The party with a lower id has the client end
// while the one with the higher id has the server end.

#include "drivacy/io/socket.h"

namespace drivacy {
namespace io {
namespace socket {

// ServerSideSocket.
void ServerSideSocket::Listen(const types::Configuration &config) {}

void ServerSideSocket::SendQuery(uint32_t party,
                                 const types::Query &query) const {}

void ServerSideSocket::SendResponse(uint32_t party,
                                    const types::Response &response) const {}

void ServerSideSocket::HandleQuery(std::string message,
                                   uint32_t client_id) const {}

// ClientSideSocket.
void ClientSideSocket::Listen(const types::Configuration &config) {}

void ClientSideSocket::SendQuery(uint32_t party,
                                 const types::Query &query) const {}

void ClientSideSocket::SendResponse(uint32_t party,
                                    const types::Response &response) const {}

void ClientSideSocket::HandleQuery(std::string message,
                                   uint32_t client_id) const {}

}  // namespace socket
}  // namespace io
}  // namespace drivacy
