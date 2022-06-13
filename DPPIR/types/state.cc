// Party state responsible for storing offline material.
#include "DPPIR/types/state.h"

#include <cassert>
#include <utility>

namespace DPPIR {

// ClientState.
// Simulated -> we are running the online stage alone with no offline stage.
// For the sake of speeding up experimentation.
// In this case, we simulate an offline state that always returns
// identity shares/tags.
// Otherwise, the state is actually created during the offline stage and
// used online.
void ClientState::Initialize(party_id_t party_count, index_t secrets,
                             bool noise, bool simulated) {
  this->simulated_ = simulated;
  this->write_idx_ = 0;
  this->read_idx_ = 0;
  this->size_ = simulated ? 1 : secrets;
  // Allocate memory.
  this->tags_ = std::make_unique<tag_t[]>(this->size_);
  this->incrementals_ =
      std::make_unique<std::vector<incremental_share_t>[]>(this->size_);
  if (!noise) {
    this->preshares_ = std::make_unique<preshare_t[]>(this->size_);
  }
  // Simulated: put a fake secret and use it always.
  if (this->simulated_) {
    this->tags_[0] = 0;
    for (party_id_t i = 0; i < party_count; i++) {
      this->incrementals_[0].push_back({0, 1});
    }
    if (!noise) {
      this->preshares_[0].fill(0);
    }
  }
}

// Storing secrets (offline).
void ClientState::AddNoiseSecret(
    const tag_t& tag, std::vector<incremental_share_t>&& incrementals) {
  this->tags_[this->write_idx_] = tag;
  this->incrementals_[this->write_idx_] = std::move(incrementals);
  this->write_idx_++;
}
void ClientState::AddSecret(const tag_t& tag,
                            std::vector<incremental_share_t>&& incrementals,
                            const preshare_t& preshare) {
  this->tags_[this->write_idx_] = tag;
  this->incrementals_[this->write_idx_] = std::move(incrementals);
  this->preshares_[this->write_idx_] = preshare;
  this->write_idx_++;
}

// Get a new secret from the stored secrets.
void ClientState::LoadNext() { this->read_idx_++; }
const tag_t& ClientState::GetTag() {
  if (this->simulated_) {
    return this->tags_[0];
  }
  return this->tags_[this->read_idx_ - 1];
}
const std::vector<incremental_share_t>& ClientState::GetIncrementalShares() {
  if (this->simulated_) {
    return this->incrementals_[0];
  }
  return this->incrementals_[this->read_idx_ - 1];
}
const preshare_t& ClientState::GetPreshare() {
  if (this->simulated_) {
    return this->preshares_[0];
  }
  return this->preshares_[this->read_idx_ - 1];
}

// Free memory when sharing is done (retain preshares for responses).
void ClientState::FinishSharing() {
  this->read_idx_ = 0;
  this->tags_ = nullptr;
  this->incrementals_ = nullptr;
}

void ClientState::Free() {
  this->write_idx_ = 0;
  this->read_idx_ = 0;
  this->size_ = 0;
  this->tags_ = nullptr;
  this->incrementals_ = nullptr;
  this->preshares_ = nullptr;
}

// PartyState.
// Simulated -> we are running the online stage alone with no offline stage.
// For the sake of speeding up experimentation.
// In this case, we simulate an offline state that always returns
// identity shares/tags.
// Otherwise, the state is actually created during the offline stage and
// used online.
void PartyState::Initialize(bool simulated) {
  this->simulated_ = simulated;
  if (this->simulated_) {
    secret& st = this->secrets_[0];
    st.next_tag = 0;
    st.incremental = {0, 1};
    st.preshare.fill(0);
  }
}

// Store secret.
void PartyState::Store(const OfflineSecret& secret) {
  auto [_, b] = this->secrets_.emplace(
      std::piecewise_construct, std::forward_as_tuple(secret.tag),
      std::forward_as_tuple(secret.next_tag, secret.share, secret.preshare));
  // Ensures no colisions.
  assert(b);
}

// Lookup offline secrets by tag.
void PartyState::LoadSecret(const tag_t& tag) {
  if (this->simulated_) {
    this->it_ = this->secrets_.find(0);
  } else {
    this->it_ = this->secrets_.find(tag);
  }
}
const tag_t& PartyState::GetNextTag() { return this->it_->second.next_tag; }
const incremental_share_t& PartyState::GetIncremental() {
  return this->it_->second.incremental;
}
const preshare_t& PartyState::GetPreshare(const tag_t& tag) {
  if (this->simulated_) {
    return this->secrets_.at(0).preshare;
  } else {
    return this->secrets_.at(tag).preshare;
  }
}

// BackendState.
// Simulated -> we are running the online stage alone with no offline stage.
// For the sake of speeding up experimentation.
// In this case, we simulate an offline state that always returns
// identity shares/tags.
// Otherwise, the state is actually created during the offline stage and
// used online.
void BackendState::Initialize(bool simulated) {
  this->simulated_ = simulated;
  if (this->simulated_) {
    secret& st = this->secrets_[0];
    st.incremental = {0, 1};
    st.preshare.fill(0);
  }
}

// Store secret.
void BackendState::Store(const OfflineSecret& secret) {
  auto [_, b] = this->secrets_.emplace(
      std::piecewise_construct, std::forward_as_tuple(secret.tag),
      std::forward_as_tuple(secret.share, secret.preshare));
  // Ensures no colisions.
  assert(b);
}

// Lookup offline secrets by tag.
void BackendState::LoadSecret(const tag_t& tag) {
  if (this->simulated_) {
    this->it_ = this->secrets_.find(0);
  } else {
    this->it_ = this->secrets_.find(tag);
  }
}
const incremental_share_t& BackendState::GetIncremental() {
  return this->it_->second.incremental;
}
const preshare_t& BackendState::GetPreshare() {
  return this->it_->second.preshare;
}

}  // namespace DPPIR
