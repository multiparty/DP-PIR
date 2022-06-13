#include <cassert>
#include <utility>

#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/client/client.h"

namespace DPPIR {
namespace protocol {

// Client party.
Client::Client(server_id_t server_id, config::Config&& config, Database&& db)
    : server_id_(server_id),
      party_count_(config.party_count),
      // Socket to first party only
      next_(onion::CipherSize(party_count_)),
      // Configuration and database.
      config_(std::move(config)),
      db_(std::move(db)),
      // Offline state.
      state_(),
      // Primary keys.
      pkeys_() {
  assert(this->party_count_ >= 2);
  // Primary keys.
  for (config::PartyConfig& party : this->config_.parties) {
    this->pkeys_.push_back(party.onion_pkey);
  }
  // Initialize socket.
  const config::ServerConfig& conf =
      this->config_.parties.at(0).servers.at(this->server_id_);
  this->next_.Initialize(conf.ip, conf.port);
}

}  // namespace protocol
}  // namespace DPPIR
