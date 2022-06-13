#include <iostream>

#include "DPPIR/protocol/backend/backend.h"

namespace DPPIR {
namespace protocol {

void BackendParty::CollectAndInstallSecrets() {
  // Listen to offline queries.
  std::cout << "Listening to " << this->queries_.Capacity()
            << " offline secrets..." << std::endl;
  index_t count = this->queries_.Capacity();
  while (count > 0) {
    CipherLogicalBuffer& buffer = this->back_.ReadCiphers(count);
    for (char* cipher : buffer) {
      this->HandleOnionCipher(cipher);
    }
    count -= buffer.Size();
    buffer.Clear();
  }
}

void BackendParty::BroadcastSecrets() {
  std::cout << "Broadcasting secrets..." << std::endl;

  // Figure out how many secrets to read from each server.
  this->siblings_.BroadcastCount(this->state_.size());

  index_t total_count = 0;
  ServersMap<index_t> from_counts(this->server_id_, this->server_count_);
  for (server_id_t id = 0; id < this->server_count_; id++) {
    if (id != this->server_id_) {
      from_counts[id] = this->siblings_.ReadCount(id);
      total_count += from_counts[id];
    }
  }

  // Broadcast all of the secrets to parallel backend.
  BackendState copy = this->state_;
  using SecretPair = BackendState::const_iterator::value_type;
  size_t poll_rate = POLL_RATE / sizeof(OfflineSecret);
  this->SendAndPoll<OfflineSecret, SecretPair, BackendState>(
      &copy, poll_rate, copy.size(), total_count, from_counts,
      std::function<void(const SecretPair&)>([this](const SecretPair& s) {
        this->siblings_.BroadcastSecret(
            {s.first, 0, s.second.incremental, s.second.preshare});
      }),
      std::function<index_t(server_id_t, index_t)>(
          [this](server_id_t source, index_t remaining) {
            LogicalBuffer<OfflineSecret>& buffer =
                this->siblings_.ReadSecrets(source, remaining);
            for (OfflineSecret& secret : buffer) {
              this->state_.Store(secret);
            }
            index_t size = buffer.Size();
            this->received_from_sibling_counts_[source] += size;
            buffer.Clear();
            return size;
          }));
}

void BackendParty::StartOffline() {
  // Allocate memory and read batch size.
  this->InitializeBatch();
  this->state_.Initialize(false);
  this->back_.SendReady();

  // Initialize offline state.
  this->CollectAndInstallSecrets();
  if (this->server_count_ > 1) {
    this->BroadcastSecrets();
    this->siblings_.BroadcastReady();
    this->siblings_.WaitForReady();
  }

  // Let previous party know we are ready to accept queries.
  this->back_.SendReady();
}

void BackendParty::SimulateOffline() {
  // Read number of queries expected in this batch.
  this->InitializeBatch();

  // Simulate offline state.
  this->state_.Initialize(true);

  // Let previous party know we are ready to accept queries.
  this->back_.SendReady();
}

}  // namespace protocol
}  // namespace DPPIR
