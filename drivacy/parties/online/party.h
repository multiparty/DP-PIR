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

#ifndef DRIVACY_PARTIES_ONLINE_PARTY_H_
#define DRIVACY_PARTIES_ONLINE_PARTY_H_

// #define DEBUG_MSG

#include <cassert>
#include <cstdint>
#include <vector>

#include "drivacy/io/interparty_socket.h"
#include "drivacy/io/intraparty_socket.h"
#include "drivacy/io/unified_listener.h"
#include "drivacy/protocol/online/shuffle.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {
namespace online {

class Party : public io::socket::InterPartySocketListener,
              public io::socket::IntraPartySocketListener {
 public:
  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  Party(uint32_t party_id, uint32_t machine_id,
        const types::Configuration &config, const types::Table &table,
        double span, double cutoff)
      : party_id_(party_id),
        machine_id_(machine_id),
        config_(config),
        table_(table),
        span_(span),
        cutoff_(cutoff),
        inter_party_socket_(party_id, machine_id, config, this),
        intra_party_socket_(party_id, machine_id, config, this),
        input_batch_size_(0),
        output_batch_size_(0),
        noise_size_(0),
        shuffler_(party_id, machine_id, config.parties(),
                  config.parallelism()) {
    // virtual funtion binds to correct subclass.
    this->listener_.AddSocket(&this->inter_party_socket_);
    this->listener_.AddSocket(&this->intra_party_socket_);
  }

  // Not movable or copyable: when an instance is constructed, a pointer to it
  // is stored in the socket (e.g. look at io/simulated_socket.[cc|h]).
  // If the object is later moved or copied, the pointer might become invalid.
  Party(Party &&other) = delete;
  Party &operator=(Party &&other) = delete;
  Party(const Party &) = delete;
  Party &operator=(const Party &) = delete;

  // Called to start the listening on the socket (blocking!)
  virtual void Start();

  // Implementation of InterPartySocketListener, these functions are called by
  // the socket when a batch size, query, or response is received on the socket.
  void OnReceiveBatchSize(uint32_t batch_size) override;
  void OnReceiveQuery(const types::Query &query) override;
  void OnReceiveResponse(const types::Response &response) override;

  // Implementation of IntraPartySocketListener, these functions are called
  // by the intra party socket when the corresponding event occurs.
  bool OnReceiveBatchSize(uint32_t machine_id, uint32_t batch_size) override;
  void OnCollectedBatchSizes() override;
  void OnReceiveQuery(uint32_t machine_id,
                      const types::Query &query) override;
  void OnReceiveResponse(uint32_t machine_id,
                         const types::Response &response) override;

  // Messages only appear in the offline stage.
  void OnReceiveMessage(const types::CipherText &message) override {
    assert(false);
  }
  void OnReceiveMessage(uint32_t machine_id,
                        const types::CipherText &message) override {
    assert(false);
  }

 protected:
  uint32_t party_id_;
  uint32_t machine_id_;
  const types::Configuration &config_;
  const types::Table &table_;
  // DP noise parameters.
  double span_;
  double cutoff_;
  // Sockets.
  io::listener::UnifiedListener listener_;
  io::socket::InterPartyTCPSocket inter_party_socket_;
  io::socket::IntraPartyTCPSocket intra_party_socket_;
  uint32_t input_batch_size_;
  uint32_t output_batch_size_;
  // Noise used in this batch.
  std::vector<types::Query> noise_;
  uint32_t noise_size_;
  // Shuffler (for distrbuted 2 phase shuffling).
  protocol::online::Shuffler shuffler_;
  // Common reference lists and maps produced by offline stage.
  std::vector<std::vector<types::Message>> commons_list_;
  types::CommonReferenceMap commons_map_;
  // Send the processed queries/responses over socket.
  virtual void SendQueries();
  virtual void SendResponses();
  virtual void InjectNoise();
};

}  // namespace online
}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_ONLINE_PARTY_H_
