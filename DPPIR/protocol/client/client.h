#ifndef DPPIR_PROTOCOL_CLIENT_CLIENT_H_
#define DPPIR_PROTOCOL_CLIENT_CLIENT_H_

#include <memory>
#include <vector>

#include "DPPIR/config/config.h"
#include "DPPIR/sockets/client_socket.h"
#include "DPPIR/types/database.h"
#include "DPPIR/types/state.h"
#include "DPPIR/types/types.h"

namespace DPPIR {
namespace protocol {

class Client {
 public:
  Client(server_id_t server_id, config::Config&& config, Database&& db);

  // Start the protocol.
  void Start(index_t count, bool offline, bool online) {
    if (offline) {
      this->StartOffline(count);
    } else {
      this->SimulateOffline(count);
    }
    if (online) {
      this->StartOnline(count);
    }
  }

 private:
  server_id_t server_id_;
  party_id_t party_count_;
  // Socket to the frontend party.
  sockets::ClientSocket next_;
  // Network and protocol configuration.
  config::Config config_;
  // The database (for validation).
  Database db_;
  // Offline state.
  ClientState state_;
  // Primary Keys.
  std::vector<pkey_t> pkeys_;
  // How many queries we will be making.
  index_t queries_count_;

  // Offline.
  void StartOffline(index_t count);
  void SimulateOffline(index_t count);

  // Online.
  // Either StartOffline() or SimulateOffline() should be called
  // before StartOnline().
  void StartOnline(index_t count);

  // Handlers.
  tag_t SampleTag(index_t id);
  std::unique_ptr<OfflineSecret[]> MakeSecret(index_t id);
  Query MakeQuery(key_t key);
  void ReconstructResponse(Response* response);
};

}  // namespace protocol
}  // namespace DPPIR

#endif  // DPPIR_PROTOCOL_CLIENT_CLIENT_H_
