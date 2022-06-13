// Party state responsible for storing offline material.
#ifndef DPPIR_TYPES_STATE_H_
#define DPPIR_TYPES_STATE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "DPPIR/types/types.h"

namespace DPPIR {

class ClientState {
 public:
  ClientState() = default;
  // Simulated -> we are running the online stage alone with no offline stage.
  // For the sake of speeding up experimentation.
  // In this case, we simulate an offline state that always returns
  // identity shares/tags.
  // Otherwise, the state is actually created during the offline stage and
  // used online.
  void Initialize(party_id_t party_count, index_t secrets, bool noise,
                  bool simulated);

  // Storing secrets (offline).
  void AddNoiseSecret(const tag_t& tag,
                      std::vector<incremental_share_t>&& incrementals);
  void AddSecret(const tag_t& tag,
                 std::vector<incremental_share_t>&& incrementals,
                 const preshare_t& preshare);

  // Get a new secret from the stored secrets.
  void LoadNext();
  const tag_t& GetTag();
  const std::vector<incremental_share_t>& GetIncrementalShares();
  const preshare_t& GetPreshare();

  // Free memory when sharing is done (retain preshares for responses).
  void FinishSharing();
  void Free();

 private:
  bool simulated_;
  // Indices.
  index_t write_idx_;
  index_t read_idx_;
  index_t size_;
  // Memory.
  std::unique_ptr<tag_t[]> tags_;
  std::unique_ptr<std::vector<incremental_share_t>[]> incrementals_;
  std::unique_ptr<preshare_t[]> preshares_;
};

class PartyState {
 public:
  // Stored secret.
  struct secret {
    tag_t next_tag;
    incremental_share_t incremental;
    preshare_t preshare;
    // Constructor for emplace.
    secret() = default;
    secret(const tag_t& n, const incremental_share_t& i, const preshare_t& p)
        : next_tag(n), incremental(i), preshare(p) {}
  };

  PartyState() = default;
  // Simulated -> we are running the online stage alone with no offline stage.
  // For the sake of speeding up experimentation.
  // In this case, we simulate an offline state that always returns
  // identity shares/tags.
  // Otherwise, the state is actually created during the offline stage and
  // used online.
  void Initialize(bool simulated);

  // Store secret.
  void Store(const OfflineSecret& secret);

  // Lookup offline secrets by tag.
  void LoadSecret(const tag_t& tag);
  const tag_t& GetNextTag();
  const incremental_share_t& GetIncremental();
  const preshare_t& GetPreshare(const tag_t& tag);

  // Iteration over all installed secrets.
  using const_iterator = std::unordered_map<tag_t, secret>::const_iterator;
  const_iterator begin() const { return this->secrets_.begin(); }
  const_iterator end() const { return this->secrets_.end(); }
  index_t size() const { return this->secrets_.size(); }

 private:
  bool simulated_;
  // storage.
  std::unordered_map<tag_t, secret> secrets_;
  // cache iterator when LoadSecret() is called for following calls to
  // GetNextTag() and GetIncremental().
  const_iterator it_;
};

// Similar to PartyState, but has no preshares!
class BackendState {
 public:
  // Stored secret.
  struct secret {
    incremental_share_t incremental;
    preshare_t preshare;
    // Constructor for emplace.
    secret() = default;
    secret(const incremental_share_t& i, const preshare_t& p)
        : incremental(i), preshare(p) {}
  };

  BackendState() = default;
  // Simulated -> we are running the online stage alone with no offline stage.
  // For the sake of speeding up experimentation.
  // In this case, we simulate an offline state that always returns
  // identity shares/tags.
  // Otherwise, the state is actually created during the offline stage and
  // used online.
  void Initialize(bool simulated);

  // Store secret.
  void Store(const OfflineSecret& secret);

  // Lookup offline secrets by tag.
  void LoadSecret(const tag_t& tag);
  const incremental_share_t& GetIncremental();
  const preshare_t& GetPreshare();

  // Iteration over all installed secrets.
  using const_iterator = std::unordered_map<tag_t, secret>::const_iterator;
  const_iterator begin() const { return this->secrets_.begin(); }
  const_iterator end() const { return this->secrets_.end(); }
  index_t size() const { return this->secrets_.size(); }

 private:
  bool simulated_;
  // storage.
  std::unordered_map<tag_t, secret> secrets_;
  // cache iterator when LoadSecret() is called for following calls to
  // GetIncremental() and GetPreshare().
  const_iterator it_;
};

}  // namespace DPPIR

#endif  // DPPIR_TYPES_STATE_H_
