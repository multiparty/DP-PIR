#include <memory>
#include <utility>
#include <vector>

#include "DPPIR/protocol/client/client.h"
#include "DPPIR/sharing/additive.h"
#include "DPPIR/sharing/incremental.h"

namespace DPPIR {
namespace protocol {

// Randomly sample a tag. Must be from a large enough domain to avoid colisions.
tag_t Client::SampleTag(index_t id) {
  // The below leaks information but is helpful for debugging as it allows us
  // to determine the source of a message via its tag.
  // TODO(babman): sample this tag uniformly at random.
  return this->server_id_ * this->queries_count_ + id;
}

// Samples an offline secret, stores it in state, and returns it for
// use in the offline protocol.
std::unique_ptr<OfflineSecret[]> Client::MakeSecret(index_t id) {
  // Sample secret components.
  tag_t tag = this->SampleTag(id);
  std::vector<incremental_share_t> incrementals =
      sharing::PreIncrementalSecretShares(this->party_count_);
  std::vector<preshare_t> preshares =
      sharing::GenerateAdditiveSecretShares(this->party_count_ + 1);

  // Construct the secret of each party
  std::unique_ptr<OfflineSecret[]> secrets =
      std::make_unique<OfflineSecret[]>(this->party_count_);
  for (party_id_t party_id = 0; party_id < this->party_count_; party_id++) {
    OfflineSecret& secret = secrets[party_id];
    secret.tag = tag;
    secret.next_tag = this->SampleTag(id);
    secret.share = incrementals.at(party_id);
    secret.preshare = preshares.at(party_id);
    tag = secret.next_tag;
  }

  // Store relevant portion in client state.
  this->state_.AddSecret(tag, std::move(incrementals),
                         preshares.at(this->party_count_));

  // Done!
  return secrets;
}

// Makes a query using an offline secret.
Query Client::MakeQuery(key_t key) {
  // Load an offline secret for this query.
  this->state_.LoadNext();
  // Query components.
  const tag_t& tag = this->state_.GetTag();
  incremental_tally_t tally = sharing::GenerateIncrementalTally(
      key, this->state_.GetIncrementalShares());
  return {tag, tally};
}

// Reconstructs a response (in place).
void Client::ReconstructResponse(Response* response) {
  // Load offline secret of this response.
  this->state_.LoadNext();
  // Reconstruct.
  sharing::AdditiveReconstruct(*response, this->state_.GetPreshare(), response);
}

}  // namespace protocol
}  // namespace DPPIR
