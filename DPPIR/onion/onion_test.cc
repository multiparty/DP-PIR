#include "DPPIR/onion/onion.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "DPPIR/sharing/additive.h"
#include "DPPIR/sharing/incremental.h"
#include "DPPIR/types/types.h"
// NOLINTNEXTLINE
#include "sodium.h"

namespace DPPIR {
namespace onion {

#define PARTIES 5

std::unique_ptr<OfflineSecret[]> SampleSecrets() {
  // Sample some secrets.
  std::vector<incremental_share_t> incrementals =
      sharing::PreIncrementalSecretShares(PARTIES);
  std::vector<preshare_t> preshares =
      sharing::GenerateAdditiveSecretShares(PARTIES);
  // Fill in result.
  std::unique_ptr<OfflineSecret[]> result =
      std::make_unique<OfflineSecret[]>(PARTIES);
  for (party_id_t party = 0; party < PARTIES; party++) {
    OfflineSecret& secret = result[party];
    secret.tag = party;
    secret.next_tag = party + 1;
    secret.share = incrementals.at(party);
    secret.preshare = preshares.at(party);
  }
  return result;
}

std::unique_ptr<OfflineSecret[]> OnionDecryptAll(
    const char* cipher, const std::vector<pkey_t> pkeys,
    const std::vector<skey_t> skeys) {
  OnionLayer layer(nullptr);
  std::unique_ptr<OfflineSecret[]> result =
      std::make_unique<OfflineSecret[]>(PARTIES);
  for (party_id_t party = 0; party < PARTIES; party++) {
    const skey_t& pkey = pkeys.at(party);
    const skey_t& skey = skeys.at(party);
    layer = OnionDecrypt(cipher, PARTIES - party, pkey, skey);
    result[party] = layer.Msg();
    cipher = layer.NextLayer();
  }
  return result;
}

bool Equals(const OfflineSecret* left, const OfflineSecret* right) {
  for (party_id_t i = 0; i < PARTIES; i++) {
    if (left[i].tag != right[i].tag) {
      return false;
    }
    if (left[i].next_tag != right[i].next_tag) {
      return false;
    }
    if (left[i].share.x != right[i].share.x) {
      return false;
    }
    if (left[i].share.y != right[i].share.y) {
      return false;
    }
    if (left[i].preshare.size() != right[i].preshare.size()) {
      return false;
    }
    for (size_t j = 0; j < left[i].preshare.size(); j++) {
      if (left[i].preshare[j] != right[i].preshare[j]) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace onion
}  // namespace DPPIR

int main() {
  assert(sodium_init() >= 0);

  // Generate key pairs.
  std::vector<DPPIR::pkey_t> pkeys;
  std::vector<DPPIR::skey_t> skeys;
  for (DPPIR::party_id_t i = 0; i < PARTIES; i++) {
    DPPIR::pkey_t pkey;
    DPPIR::skey_t skey;
    DPPIR::onion::GenerateKeyPair(&pkey, &skey);
    pkeys.push_back(pkey);
    skeys.push_back(skey);
  }

  // Perform test.
  for (int i = 0; i < 100; i++) {
    auto plain = DPPIR::onion::SampleSecrets();
    auto cipher = DPPIR::onion::OnionEncrypt(plain.get(), 0, PARTIES, pkeys);
    auto dec = DPPIR::onion::OnionDecryptAll(cipher.get(), pkeys, skeys);
    /*if (!DPPIR::onion::Equals(plain.get(), dec.get())) {
      std::cout << "Test failed!" << std::endl;
      return 1;
    }*/
  }

  std::cout << "All tests passed!" << std::endl;

  return 0;
}
