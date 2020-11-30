// Copyright 2020 multiparty.org

// This file defines the "BackendParty" class. A specialized Party that
// sits at the deepest level in the protocol chain and responds to queries
// through the table.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#ifndef DRIVACY_PARTIES_OFFLINE_BACKEND_PARTY_H_
#define DRIVACY_PARTIES_OFFLINE_BACKEND_PARTY_H_

#include <cassert>
#include <cstdint>

#include "drivacy/parties/offline/party.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {
namespace offline {

// HeadParty is the special first party, which is responsible for communicating
// with other parties as well as clients!
class BackendParty : public Party {
 public:
  BackendParty(uint32_t party, uint32_t machine,
               const types::Configuration &config, const types::Table &table,
               double span, double cutoff)
      : Party(party, machine, config, table, span, cutoff),
        processed_messages_(0) {}

  // Not copyable or movable!
  BackendParty(BackendParty &&other) = delete;
  BackendParty &operator=(BackendParty &&other) = delete;
  BackendParty(const BackendParty &) = delete;
  BackendParty &operator=(const BackendParty &) = delete;

  void Start() override;

  void OnReceiveBatchSize(uint32_t batch_size) override;
  void OnReceiveMessage(const types::CipherText &message) override;

  // Backend does not use any intra-party communication.
  bool OnReceiveBatchSize(uint32_t machine_id, uint32_t batch_size) override {
    assert(false);
  }
  void OnCollectedBatchSizes() override { assert(false); }
  void OnReceiveMessage(uint32_t machine_id,
                        const types::CipherText &message) override {
    assert(false);
  }

 protected:
  // Count how many queries have been processed in this batch.
  uint32_t processed_messages_;
  // No next party to send anything to.
  void SendMessages() override { assert(false); }
  // We do not inject noise.
  void InjectNoise() override { assert(false); }
};

}  // namespace offline
}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_OFFLINE_BACKEND_PARTY_H_
