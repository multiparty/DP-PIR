// Copyright 2020 multiparty.org

// This file defines the "BackendParty" class. A specialized Party that
// sits at the deepest level in the protocol chain and responds to queries
// through the table.
//
// All comments about deploying/running parties in simulation or deployment
// in drivacy/parties/party.h apply here too.

#ifndef DRIVACY_PARTIES_BACKEND_PARTY_H_
#define DRIVACY_PARTIES_BACKEND_PARTY_H_

#include <cstdint>

#include "drivacy/parties/party.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {

// HeadParty is the special first party, which is responsible for communicating
// with other parties as well as clients!
class BackendParty : public Party {
 public:
  BackendParty(uint32_t party, uint32_t machine,
               const types::Configuration &config, const types::Table &table,
               io::socket::SocketFactory socket_factory);

  // Not copyable or movable!
  BackendParty(BackendParty &&other) = delete;
  BackendParty &operator=(BackendParty &&other) = delete;
  BackendParty(const BackendParty &) = delete;
  BackendParty &operator=(const BackendParty &) = delete;

  void OnReceiveBatch(uint32_t batch_size) override;

  void OnReceiveQuery(const types::IncomingQuery &query) override;

  // Backend party never receives responses or uses intra-party communication.
  void OnQueriesReady(uint32_t machine_id) override { assert(false); }
  void OnResponsesReady(uint32_t machine_id) override { assert(false); }

  void OnReceiveResponse(const types::ForwardResponse &response) override {
    assert(false);
  }
  bool OnReceiveQuery(uint32_t machine_id,
                      const types::ForwardQuery &query) override {
    assert(false);
  }
  bool OnReceiveResponse(uint32_t machine_id,
                         const types::Response &response) override {
    assert(false);
  }
  void SendQueries() override { assert(false); }
  void SendResponses() override { assert(false); }

 protected:
  uint32_t processed_queries_;
};

}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_BACKEND_PARTY_H_
