// Copyright 2020 multiparty.org

// A (fake/simulated) socket-like interface for simulating over the wire
// communication, when running all the parties locally for testing/debugging.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#ifndef DRIVACY_IO_SIMULATED_SOCKET_H_
#define DRIVACY_IO_SIMULATED_SOCKET_H_

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

class SimulatedSocket : public AbstractSocket {
 public:
  SimulatedSocket(uint32_t party_id, QueryListener query_listener,
                  ResponseListener response_listener,
                  const types::Configuration &config);

  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, QueryListener query_listener,
      ResponseListener response_listener, const types::Configuration &config) {
    return std::make_unique<SimulatedSocket>(party_id, query_listener,
                                             response_listener, config);
  }

  void SendQuery(const types::OutgoingQuery &query) override;
  void SendResponse(const types::Response &response) override;

  void Listen() override {}

 private:
  static std::unordered_map<uint32_t, SimulatedSocket *> sockets_;
};

// Similar to the above class, but used by the first server/party to simulate
// communication with clients.
class SimulatedClientSocket : public AbstractSocket {
 public:
  SimulatedClientSocket(uint32_t party_id, QueryListener query_listener,
                        ResponseListener response_listener,
                        const types::Configuration &config);

  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, QueryListener query_listener,
      ResponseListener response_listener, const types::Configuration &config) {
    return std::make_unique<SimulatedClientSocket>(party_id, query_listener,
                                                   response_listener, config);
  }

  void SendQuery(const types::OutgoingQuery &query) override;
  void SendResponse(const types::Response &response) override;

  void Listen() override {}

 private:
  static std::unordered_map<uint32_t, SimulatedClientSocket *> sockets_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_SIMULATED_SOCKET_H_
