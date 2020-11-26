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

// #define DEBUG_MSG

#include <cstdint>
#include <list>
#include <memory>

#include "drivacy/io/abstract_socket.h"
#include "drivacy/protocol/shuffle.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {

class Party : public io::socket::SocketListener,
              public io::socket::IntraPartySocketListener {
 public:
  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  Party(uint32_t party_id, uint32_t machine_id,
        const types::Configuration &config, const types::Table &table,
        double span, double cutoff, io::socket::SocketFactory socket_factory,
        io::socket::IntraPartySocketFactory intra_party_socket_factory)
      : party_id_(party_id),
        machine_id_(machine_id),
        config_(config),
        table_(table),
        span_(span),
        cutoff_(cutoff),
        batch_size_(0),
        queries_shuffled_(0),
        responses_deshuffled_(0),
        processed_queries_(0),
        processed_responses_(0),
        query_machines_ready_(0),
        response_machines_ready_(0),
        noise_size_(0),
        shuffler_(party_id, machine_id, config.parties(),
                  config.parallelism()) {
    // virtual funtion binds to correct subclass.
    this->socket_ = socket_factory(party_id, machine_id, config, this);
    this->intra_party_socket_ =
        intra_party_socket_factory(party_id, machine_id, config, this);
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
  void OnReceiveBatchSize(uint32_t batch_size) override;
  void OnReceiveQuery(const types::IncomingQuery &query) override;
  void OnReceiveResponse(const types::ForwardResponse &response) override;

  // Implementation of IntraPartySocketListener, these functions are called
  // by the intra party socket when the corresponding event occurs.
  bool OnReceiveBatchSize(uint32_t machine_id, uint32_t batch_size) override;
  void OnReceiveBatchSize2() override;
  void OnReceiveQuery(uint32_t machine_id,
                      const types::ForwardQuery &query) override;
  void OnReceiveResponse(uint32_t machine_id,
                         const types::Response &response) override;
  void OnQueriesReady(uint32_t machine_id) override;
  void OnResponsesReady(uint32_t machine_id) override;

 protected:
  uint32_t party_id_;
  uint32_t machine_id_;
  const types::Configuration &config_;
  const types::Table &table_;
  // DP noise parameters.
  double span_;
  double cutoff_;
  // Sockets.
  std::unique_ptr<io::socket::AbstractSocket> socket_;
  std::unique_ptr<io::socket::AbstractIntraPartySocket> intra_party_socket_;
  uint32_t batch_size_;
  uint32_t queries_shuffled_;
  uint32_t responses_deshuffled_;
  // Tracks the number of processed queries and responses.
  uint32_t processed_queries_;
  uint32_t processed_responses_;
  uint32_t query_machines_ready_;
  uint32_t response_machines_ready_;
  // Noise used in this batch.
  std::list<types::OutgoingQuery> noise_;
  uint32_t noise_size_;
  // Shuffler (for distrbuted 2 phase shuffling).
  protocol::Shuffler shuffler_;
  // Send the processed queries/responses over socket.
  virtual void SendQueries();
  virtual void SendResponses();
};

}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_PARTY_H_
