// Copyright 2020 multiparty.org

// A (fake/simulated) socket-like interface for simulating over the wire
// communication, when running all the parties locally for testing/debugging.
//
// The parties are all run in the same process/thread locally. They are merely
// logical entities. They communicate via this file's API, which is identical
// to the API of the real over-the-wire socket API.

#ifndef DRIVACY_IO_SIMULATED_SOCKET_H_
#define DRIVACY_IO_SIMULATED_SOCKET_H_

#include <cstdlib>
#include <unordered_map>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/messages.pb.h"

namespace drivacy {
namespace io {
namespace socket {

using QueryListener =
    std::function<void(uint32_t, const drivacy::types::Query &)>;

using ResponseListener =
    std::function<void(uint32_t, const drivacy::types::Response &)>;

class SimulatedSocket : public AbstractSocket {
 public:
  SimulatedSocket(uint32_t party_id, QueryListener query_listener,
                  ResponseListener response_listener);
  ~SimulatedSocket() override;

  void SendQuery(uint32_t party,
                 const drivacy::types::Query &query) const override;
  void SendResponse(uint32_t party,
                    const drivacy::types::Response &response) const override;

 private:
  static std::unordered_map<uint32_t, SimulatedSocket *> sockets_;

  uint32_t party_id_;
  QueryListener query_listener_;
  ResponseListener response_listener_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_SIMULATED_SOCKET_H_
