#include "DPPIR/onion/onion.h"
#include "DPPIR/protocol/backend/backend.h"
#include "DPPIR/sharing/additive.h"
#include "DPPIR/sharing/incremental.h"

namespace DPPIR {
namespace protocol {

void BackendParty::HandleOnionCipher(const char* cipher) {
  // Decrypt cipher.
  onion::OnionLayer layer =
      onion::OnionDecrypt(cipher, 1, this->party_config_.onion_pkey,
                          this->party_config_.onion_skey);

  // Install secret.
  this->state_.Store(layer.Msg());
}

Response BackendParty::HandleQuery(const Query& query) {
  // Load corresponding offline secret by tag.
  this->state_.LoadSecret(query.tag);

  // Reconstruct the database key.
  const incremental_share_t& incremental = this->state_.GetIncremental();
  key_t key = sharing::IncrementalReconstruct(query.tally, incremental);

  // Lookup response.
  const Response& value = this->db_.Lookup(key);

  // Mask response.
  Response response;
  sharing::AdditiveReconstruct(value, this->state_.GetPreshare(), &response);

  return response;
}

}  // namespace protocol
}  // namespace DPPIR
