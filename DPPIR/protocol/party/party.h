#ifndef DPPIR_PROTOCOL_PARTY_PARTY_H_
#define DPPIR_PROTOCOL_PARTY_PARTY_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "DPPIR/config/config.h"
#include "DPPIR/noise/noise.h"
#include "DPPIR/shuffle/local_shuffle.h"
#include "DPPIR/sockets/client_socket.h"
#include "DPPIR/sockets/server_socket.h"
#include "DPPIR/types/containers.h"
#include "DPPIR/types/database.h"
#include "DPPIR/types/state.h"
#include "DPPIR/types/types.h"

namespace DPPIR {
namespace protocol {

class Party {
 public:
  Party(server_id_t server_id, party_id_t party_id, config::Config&& config,
        Database&& db);

  // Start the protocol.
  void Start(bool offline, bool online) {
    if (offline) {
      this->StartOffline();
    } else {
      this->SimulateOffline();
    }
    if (online) {
      this->StartOnline();
    }
  }

 protected:
  // party this server belongs to.
  party_id_t party_id_;
  server_id_t server_id_;
  party_id_t party_count_;
  server_id_t server_count_;
  // Socket from previous party (or client if party_id_ == 0).
  sockets::ServerSocket back_;
  // Socket to next party.
  sockets::ClientSocket next_;
  // Network and protocol configuration.
  config::Config config_;
  config::PartyConfig& party_config_;
  config::ServerConfig& server_config_;
  // Database.
  Database db_;
  // Shufflers.
  shuffle::LocalShuffler lshuffler_;  // Shuffler within this server.
  // Counts.
  index_t noise_count_;
  index_t input_count_;
  index_t shuffled_count_;
  // Batch storage.
  Batch<sample_t> noise_;  // # of noise queries per DB key.
  CipherBatch ciphers_;
  Batch<tag_t> tags_;
  Batch<Query> queries_;
  Batch<Response> responses_;
  // Offline state.
  PartyState queries_state_;  // Installed by clients/previous parties.
  ClientState noise_state_;   // Create by this party for noise.
  // Sampler.
  noise::NoiseDistribution distribution_;
  // Noise domain.
  key_t noise_start_;
  key_t noise_end_;
  // Onion encryption keys.
  std::vector<pkey_t> pkeys_;

  // Protocol steps.
  // Initialization: these steps should be done offline.
  void InitializeNoiseSamples();
  void InitializeCounts();
  void InitializeShuffler();
  void InitializeNoiseQueries();

  // Offline steps.
  void CollectCiphers();
  void CreateNoiseCiphers();
  void InstallSecrets();
  void SendCiphers();
  void StartOffline();
  void SimulateOffline();

  // Online steps.
  void CollectQueries();    // Collect queries from previous party or client.
  void SendQueries();       // Handle the queries and send them to next party.
  void CollectResponses();  // Collect and handle responses from next party.
  void SendResponses();     // Send responses to previous party or client.
  // Either StartOffline() or SimulateOffline() should be called
  // before StartOnline().
  void StartOnline();

  // Handlers.
  tag_t SampleTag(index_t id);
  std::unique_ptr<OfflineSecret[]> MakeNoiseSecret(index_t id);
  void MakeNoiseQuery(key_t key, Query* target);
  void HandleQuery(const Query& input, Query* target);
  void HandleResponse(const tag_t& tag, const Response& input,
                      Response* target);
};

}  // namespace protocol
}  // namespace DPPIR

#endif  // DPPIR_PROTOCOL_PARTY_PARTY_H_
