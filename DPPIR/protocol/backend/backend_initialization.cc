#include <cassert>
#include <utility>

#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/backend/backend.h"

namespace DPPIR {
namespace protocol {

// Backend party.
BackendParty::BackendParty(server_id_t server_id, config::Config&& config,
                           Database&& db)
    : party_id_(config.party_count - 1),
      server_id_(server_id),
      server_count_(config.server_count),
      // Back socket only.
      back_(onion::CipherSize(1)),
      // Sibling information, in case we have parallel backends.
      siblings_(this->server_id_, this->server_count_, onion::CipherSize(1)),
      received_from_sibling_counts_(server_count_, server_count_ + 1, 0),
      // Configuration
      config_(std::move(config)),
      party_config_(config_.parties.at(party_id_)),
      server_config_(party_config_.servers.at(server_id_)),
      // Database.
      db_(std::move(db)),
      // Offline state.
      state_() {
  assert(config.party_count >= 2);
  // Initialize socket.
  this->back_.Initialize(this->server_config_.port);
  // Initialize siblings socket if there are any.
  if (this->server_count_ > 1) {
    this->siblings_.Initialize(this->party_config_);
  }
}

void BackendParty::InitializeBatch() {
  // Read count from previous party.
  this->queries_.Initialize(this->back_.ReadCount());
}

}  // namespace protocol
}  // namespace DPPIR
