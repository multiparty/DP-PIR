// Copyright 2020 multiparty.org

// This file defines the "BackendParty" class. A specialized Party that
// sits at the deepest level in the protocol chain and responds to queries
// through the table.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#ifndef DRIVACY_PARTIES_ONLINE_BACKEND_PARTY_H_
#define DRIVACY_PARTIES_ONLINE_BACKEND_PARTY_H_

#include <cstdint>
#include <vector>

#include "drivacy/parties/online/party.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {
namespace online {

// HeadParty is the special first party, which is responsible for communicating
// with other parties as well as clients!
class BackendParty : public Party {
 public:
  BackendParty(uint32_t party, uint32_t machine,
               const types::Configuration &config, const types::Table &table,
               double span, double cutoff)
      : Party(party, machine, config, table, span, cutoff) {}

  // Not copyable or movable!
  BackendParty(BackendParty &&other) = delete;
  BackendParty &operator=(BackendParty &&other) = delete;
  BackendParty(const BackendParty &) = delete;
  BackendParty &operator=(const BackendParty &) = delete;

  void Start() override;

  void OnReceiveBatchSize(uint32_t batch_size) override;
  void OnReceiveQuery(const types::Query &query) override;

  // Backend cannot receive responses.
  void OnReceiveResponse(const types::Response &response) override {
    assert(false);
  }

  // Backend does not use any intra-party communication.
  bool OnReceiveBatchSize(uint32_t machine_id, uint32_t batch_size) override {
    assert(false);
  }
  void OnCollectedBatchSizes() override { assert(false); }
  void OnReceiveQuery(uint32_t machine_id, const types::Query &query) override {
    assert(false);
  }
  void OnReceiveResponse(uint32_t machine_id,
                         const types::Response &response) override {
    assert(false);
  }

 protected:
  // Store processed responses of a batch.
  std::vector<types::Response> responses_;
  // Change how responses are sent.
  void SendQueries() override { assert(false); }
  void SendResponses() override;
  // We do not inject noise.
  void InjectNoise() override { assert(false); }
};

}  // namespace online
}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_ONLINE_BACKEND_PARTY_H_
