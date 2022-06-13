#ifndef DPPIR_PROTOCOL_PARALLEL_PARTY_PARALLEL_PARTY_H_
#define DPPIR_PROTOCOL_PARALLEL_PARTY_PARALLEL_PARTY_H_

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "DPPIR/config/config.h"
#include "DPPIR/noise/noise.h"
#include "DPPIR/shuffle/local_shuffle.h"
#include "DPPIR/shuffle/parallel_shuffle.h"
#include "DPPIR/sockets/client_socket.h"
#include "DPPIR/sockets/parallel_socket.h"
#include "DPPIR/sockets/server_socket.h"
#include "DPPIR/types/containers.h"
#include "DPPIR/types/database.h"
#include "DPPIR/types/state.h"
#include "DPPIR/types/types.h"

namespace DPPIR {
namespace protocol {

class ParallelParty {
 public:
  ParallelParty(server_id_t server_id, party_id_t party_id,
                config::Config&& config, Database&& db);

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

 private:
  // party this server belongs to.
  party_id_t party_id_;
  server_id_t server_id_;
  party_id_t party_count_;
  server_id_t server_count_;
  // Cipher (i.e. message) size for input/output onion ciphers (offline).
  size_t input_cipher_size_;
  size_t output_cipher_size_;
  // Socket from previous party (or client if party_id_ == 0).
  sockets::ServerSocket back_;
  // Socket to next party.
  sockets::ClientSocket next_;
  // Socket to parallel servers.
  sockets::ParallelSocket siblings_;
  // Determines message exchanges with siblings.
  // How many total messages each sibling has before shuffling (noise + input).
  ServersMap<index_t> at_sibling_counts_;
  index_t total_batch_size_;
  // How many noise queries are going to be sent from each sibling.
  ServersMap<index_t> noise_from_sibling_counts_;
  ServersMap<index_t> noise_from_sibling_prefixsum_;
  // How many messages we received from each sibling so far.
  ServersMap<index_t> received_from_sibling_counts_;
  // Network and protocol configuration.
  config::Config config_;
  config::PartyConfig& party_config_;
  config::ServerConfig& server_config_;
  // Database.
  Database db_;
  // Shufflers.
  shuffle::ParallelShuffler pshuffler_;  // Shuffler among parallel servers.
  shuffle::LocalShuffler lshuffler_;     // Shuffler within this server.
  // Counts.
  index_t noise_count_;  // # of input msgs/queries from prev. party.
  index_t input_count_;  // # of noise msgs/queries gen. by this server.
  index_t shuffled_count_;
  // Batch storage.
  Batch<sample_t> noise_;  // # of noise queries per DB key.
  CipherBatch in_ciphers_;
  CipherBatch out_ciphers_;
  Batch<tag_t> in_tags_;
  Batch<Query> in_queries_;
  Batch<Query> out_queries_;
  Batch<Response> in_responses_;
  Batch<Response> out_responses_;
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

  // Initialization: these steps should be done offline.
  void InitializeNoiseSamples();
  void InitializeCounts();
  void InitializeShufflers();
  void InitializeNoiseQueries();

  // Offline steps.
  void CollectCiphers();
  void CreateNoiseCiphers();
  void InstallSecrets();
  void ShuffleCiphers();
  void SendCiphers();
  void BroadcastSecrets();
  void StartOffline();
  void SimulateOffline();

  // Online steps.
  void CollectQueries();
  void ShuffleQueries();      // Shuffle (across servers then locally).
  void SendQueries();         // Send to next party.
  void CollectResponses();    // Read responses from next party.
  void DeshuffleResponses();  // Deshuffle (across servers and locally).
  void SendResponses();       // Send responses to previous party or client.
  // Either StartOffline() or SimulateOffline() should be called
  // before StartOnline().
  void StartOnline();

  // Shuffling helpers.
  void FromSibling(server_id_t source, const char* cipher);
  void FromSibling(server_id_t source, const Query& query);
  void FromSibling(server_id_t source, const Response& response);

  // Handlers.
  tag_t SampleTag(index_t id);
  std::unique_ptr<OfflineSecret[]> MakeNoiseSecret(index_t id);
  void MakeNoiseQuery(key_t key, Query* target);
  void HandleQuery(const Query& input, Query* target);
  void HandleResponse(const tag_t& tag, const Response& input,
                      Response* target);

#include "DPPIR/protocol/parallel_party/parallel_party_util.inc"
};

}  // namespace protocol
}  // namespace DPPIR

#endif  // DPPIR_PROTOCOL_PARALLEL_PARTY_PARALLEL_PARTY_H_
