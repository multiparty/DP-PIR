#ifndef DPPIR_PROTOCOL_BACKEND_BACKEND_H_
#define DPPIR_PROTOCOL_BACKEND_BACKEND_H_

#include <functional>
#include <iostream>

#include "DPPIR/config/config.h"
#include "DPPIR/sockets/parallel_socket.h"
#include "DPPIR/sockets/server_socket.h"
#include "DPPIR/types/containers.h"
#include "DPPIR/types/database.h"
#include "DPPIR/types/state.h"
#include "DPPIR/types/types.h"

namespace DPPIR {
namespace protocol {

class BackendParty {
 public:
  BackendParty(server_id_t server_id, config::Config&& config, Database&& db);

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
  server_id_t server_count_;
  // Socket from previous party.
  sockets::ServerSocket back_;
  // Socket to siblings if any exist (only for sharing offline secrets).
  sockets::ParallelSocket siblings_;
  // How many messages we received from each sibling so far.
  ServersMap<index_t> received_from_sibling_counts_;
  // Network and protocol configuration.
  config::Config config_;
  config::PartyConfig& party_config_;
  config::ServerConfig& server_config_;
  // The database.
  Database db_;
  // Query batch.
  Batch<Query> queries_;
  // Offline state.
  BackendState state_;

  // Initialization.
  void InitializeBatch();

  // Offline.
  void CollectAndInstallSecrets();
  void BroadcastSecrets();
  void StartOffline();
  void SimulateOffline();

  // Online.
  void CollectQueries();
  void SendResponses();
  // Either StartOffline() or SimulateOffline() should be called
  // before StartOnline().
  void StartOnline();

  // Handlers.
  void HandleOnionCipher(const char* cipher);
  Response HandleQuery(const Query& query);

#include "DPPIR/protocol/parallel_party/parallel_party_util.inc"
};

}  // namespace protocol
}  // namespace DPPIR

#endif  // DPPIR_PROTOCOL_BACKEND_BACKEND_H_
