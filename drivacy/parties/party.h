// Copyright 2020 multiparty.org

// This file defines the "Party" class, which represents
// A party/machine-specific instance of our protocol.
//
// When deployed, every machine's code will construct exactly a single
// instance of this class, and use it as the main control flow for the protocol.
//
// In a simulated environemnt, when all parties are run locally. The process
// will construct several instances of this "Party" class, one per logical
// party.

#ifndef DRIVACY_PARTIES_PARTY_H_
#define DRIVACY_PARTIES_PARTY_H_

#include <cstdint>
#include <memory>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/protocol/shuffle.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {

class Party : public io::socket::SocketListener {
 public:
  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  Party(uint32_t party_id, uint32_t machine_id,
        const types::Configuration &config, const types::Table &table,
        io::socket::SocketFactory socket_factory)
      : party_id_(party_id),
        machine_id_(machine_id),
        config_(config),
        table_(table) {
    // virtual funtion binds to correct subclass.
    this->socket_ = socket_factory(party_id, machine_id, config, this);
    this->processed_queries_ = 0;
    this->processed_responses_ = 0;
  }

  // Not movable or copyable: when an instance is constructed, a pointer to it
  // is stored in the socket (e.g. look at io/simulated_socket.[cc|h]).
  // If the object is later moved or copied, the pointer might become invalid.
  Party(Party &&other) = delete;
  Party &operator=(Party &&other) = delete;
  Party(const Party &) = delete;
  Party &operator=(const Party &) = delete;

  // Called to start the listening on the socket (blocking!)
  virtual void Listen();

  // Implementation of SocketListener, these functions are called by the
  // socket when a batch size, query, or response is received on the socket.
  void OnReceiveBatch(uint32_t batch_size) override;
  void OnReceiveQuery(const types::IncomingQuery &query) override;
  void OnReceiveResponse(const types::Response &response) override;

 protected:
  uint32_t party_id_;
  uint32_t machine_id_;
  const types::Configuration &config_;
  const types::Table &table_;
  std::unique_ptr<io::socket::AbstractSocket> socket_;
  protocol::Shuffler shuffler_;
  uint32_t batch_size_;
  // Tracks the number of processed queries and responses.
  uint32_t processed_queries_;
  uint32_t processed_responses_;
  // Send the processed queries/responses over socket.
  virtual void SendQueries();
  virtual void SendResponses();
};

}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_PARTY_H_
