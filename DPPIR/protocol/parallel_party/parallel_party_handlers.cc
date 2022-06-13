#include <utility>

#include "DPPIR/protocol/parallel_party/parallel_party.h"
#include "DPPIR/sharing/additive.h"
#include "DPPIR/sharing/incremental.h"

namespace DPPIR {
namespace protocol {

static int64_t NOISE_OFFSET = -1;

// Randomly sample a tag. Must be from a large enough domain to avoid colisions.
tag_t ParallelParty::SampleTag(index_t id) {
  // The below leaks information but is helpful for debugging as it allows us
  // to determine the source of a message via its tag.
  if (NOISE_OFFSET == -1) {
    index_t range = this->noise_end_ - this->noise_start_;
    index_t noise_per_row = this->noise_count_ / range;
    index_t total_noise = noise_per_row * this->db_.Size();
    NOISE_OFFSET = total_noise - (this->noise_start_ * noise_per_row);
  }
  // TODO(babman): sample the tag uniformly at random.
  return this->total_batch_size_ - NOISE_OFFSET + id;
}

// Samples an offline secret, stores it in state, and returns it for
// use in the offline protocol.
std::unique_ptr<OfflineSecret[]> ParallelParty::MakeNoiseSecret(index_t id) {
  party_id_t remaining_parties = this->party_count_ - this->party_id_ - 1;

  // Sample secret components.
  tag_t tag = this->SampleTag(id);
  std::vector<incremental_share_t> incrementals =
      sharing::PreIncrementalSecretShares(remaining_parties);
  std::vector<preshare_t> preshares =
      sharing::GenerateAdditiveSecretShares(remaining_parties + 1);

  // Construct the secret of each party
  std::unique_ptr<OfflineSecret[]> secrets =
      std::make_unique<OfflineSecret[]>(remaining_parties);
  for (party_id_t party_id = 0; party_id < remaining_parties; party_id++) {
    OfflineSecret& secret = secrets[party_id];
    secret.tag = tag;
    secret.next_tag = this->SampleTag(id);
    secret.share = incrementals.at(party_id);
    secret.preshare = preshares.at(party_id);
    tag = secret.next_tag;
  }

  // Store relevant portion in client state.
  this->noise_state_.AddNoiseSecret(tag, std::move(incrementals));

  // Done!
  return secrets;
}

// Make a (noise) query targeting given DB key.
void ParallelParty::MakeNoiseQuery(key_t key, Query* target) {
  // Load an offline secret to use for making the query.
  this->noise_state_.LoadNext();
  // Query components.
  target->tag = this->noise_state_.GetTag();
  target->tally = sharing::GenerateIncrementalTally(
      key, this->noise_state_.GetIncrementalShares());
}

// Handle an incoming query.
void ParallelParty::HandleQuery(const Query& input, Query* target) {
  // Load the offline secret corresponding to received query tag.
  this->queries_state_.LoadSecret(input.tag);

  // Use the secret to handle the query.
  target->tag = this->queries_state_.GetNextTag();
  target->tally = sharing::IncrementalReconstruct(
      input.tally, this->queries_state_.GetIncremental());
}

// Handle a response received from the next party.
void ParallelParty::HandleResponse(const tag_t& tag, const Response& input,
                                   Response* target) {
  // Reconstruct at the target.
  sharing::AdditiveReconstruct(input, this->queries_state_.GetPreshare(tag),
                               target);
}

}  // namespace protocol
}  // namespace DPPIR
