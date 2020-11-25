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
#include <vector>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace io {
namespace socket {

class SimulatedSocket : public AbstractSocket {
 public:
  SimulatedSocket(uint32_t party_id, uint32_t machine_id,
                  const types::Configuration &config, SocketListener *listener);

  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, uint32_t machine_id,
      const types::Configuration &config, SocketListener *listener) {
    return std::make_unique<SimulatedSocket>(party_id, machine_id, config,
                                             listener);
  }

  ~SimulatedSocket() { SimulatedSocket::sockets_.erase(this->party_id_); }

  void SendBatch(uint32_t batch_size) override;
  void SendQuery(const types::ForwardQuery &query) override;
  void SendResponse(const types::Response &response) override;

  void FlushQueries() override {}
  void FlushResponses() override {}

  void Listen() override {}

 private:
  static std::unordered_map<uint32_t,
                            std::unordered_map<uint32_t, SimulatedSocket *>>
      sockets_;
};

// Similar to the above class, but used by the first server/party to simulate
// communication with clients.
class SimulatedClientSocket : public AbstractSocket {
 public:
  SimulatedClientSocket(uint32_t party_id, uint32_t machine_id,
                        const types::Configuration &config,
                        SocketListener *listener);

  ~SimulatedClientSocket() {
    SimulatedClientSocket::sockets_.erase(this->party_id_);
  }

  static std::unique_ptr<AbstractSocket> Factory(
      uint32_t party_id, uint32_t machine_id,
      const types::Configuration &config, SocketListener *listener) {
    return std::make_unique<SimulatedClientSocket>(party_id, machine_id, config,
                                                   listener);
  }

  void SendBatch(uint32_t batch_size) override { assert(false); }
  void SendQuery(const types::ForwardQuery &query) override;
  void SendResponse(const types::Response &response) override;

  void FlushQueries() override { assert(false); }
  void FlushResponses() override { assert(false); }

  void Listen() override {}

 private:
  static std::unordered_map<
      uint32_t, std::unordered_map<uint32_t, SimulatedClientSocket *>>
      sockets_;
};

class SimulatedIntraPartySocket : public AbstractIntraPartySocket {
 public:
  SimulatedIntraPartySocket(uint32_t party_id, uint32_t machine_id,
                            const types::Configuration &config,
                            IntraPartySocketListener *listener);

  static std::unique_ptr<AbstractIntraPartySocket> Factory(
      uint32_t party_id, uint32_t machine_id,
      const types::Configuration &config, IntraPartySocketListener *listener) {
    return std::make_unique<SimulatedIntraPartySocket>(party_id, machine_id,
                                                       config, listener);
  }

  ~SimulatedIntraPartySocket() {
    std::unordered_map<uint32_t, SimulatedIntraPartySocket *> &nested_map =
        SimulatedIntraPartySocket::sockets_.at(this->party_id_);
    nested_map.erase(this->machine_id_);
    if (nested_map.empty()) {
      SimulatedIntraPartySocket::sockets_.erase(this->party_id_);
    }
  }

  void BroadcastBatchSize(uint32_t batch_size) override;
  void BroadcastQueriesReady() override;
  void BroadcastResponsesReady() override;

  void SendQuery(uint32_t machine_id,
                 const types::OutgoingQuery &query) override;
  void SendResponse(uint32_t machine_id,
                    const types::ForwardResponse &response) override;

  void FlushQueries() override {}
  void FlushResponses() override {}

  void ListenBatchSizes() override {}
  void ListenQueries(std::vector<uint32_t> counts) override {}
  void ListenResponses() override {}

 private:
  static std::unordered_map<
      uint32_t, std::unordered_map<uint32_t, SimulatedIntraPartySocket *>>
      sockets_;
};

}  // namespace socket
}  // namespace io
}  // namespace drivacy

#endif  // DRIVACY_IO_SIMULATED_SOCKET_H_
