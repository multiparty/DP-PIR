// Copyright 2020 multiparty.org

// This file defines the "Party" class, which represents
// A party/machine-specific instance of our offline protocol.

#ifndef DRIVACY_PARTIES_OFFLINE_PARTY_H_
#define DRIVACY_PARTIES_OFFLINE_PARTY_H_

// #define DEBUG_MSG

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "drivacy/io/interparty_socket.h"
#include "drivacy/io/intraparty_socket.h"
#include "drivacy/io/unified_listener.h"
#include "drivacy/protocol/offline/noise.h"
#include "drivacy/protocol/offline/shuffle.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {
namespace offline {

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
        shuffler_(party_id, machine_id, config.parties(),
                  config.parallelism()) {
    // It is known how much noise we need to add.
    this->noise_size_ = protocol::offline::noise::UpperBound(
        machine_id, config.parallelism(), table.size(), span, cutoff);
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
  void OnReceiveMessage(const types::CipherText &message) override;

  // Implementation of IntraPartySocketListener, these functions are called
  // by the intra party socket when the corresponding event occurs.
  bool OnReceiveBatchSize(uint32_t machine_id, uint32_t batch_size) override;
  void OnCollectedBatchSizes() override;
  void OnReceiveMessage(uint32_t machine_id,
                        const types::CipherText &message) override;

  // This is the offline stage, nothing related to queries or responses
  // is allowed.
  void OnReceiveQuery(const types::Query &query) override { assert(false); }
  void OnReceiveResponse(const types::Response &response) override {
    assert(false);
  }
  void OnReceiveQuery(uint32_t machine_id, const types::Query &query) override {
    assert(false);
  }
  void OnReceiveResponse(uint32_t machine_id,
                         const types::Response &response) override {
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
  uint32_t noise_size_;
  // Shuffler (for distrbuted 2 phase shuffling).
  protocol::offline::Shuffler shuffler_;
  // Stores all installed common reference messages.
  std::unordered_map<types::Tag, types::CommonReference> common_references_;
  // Send the processed messages over socket.
  virtual void SendMessages();
  virtual void InjectNoise();
  virtual void SaveCommonReference();
};

}  // namespace offline
}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_OFFLINE_PARTY_H_
