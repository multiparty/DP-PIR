// Onion Encryption.
#include "DPPIR/onion/onion.h"

#include <cassert>
#include <cstring>

// NOLINTNEXTLINE
#include "sodium.h"

namespace DPPIR {
namespace onion {

#define CAST(buff) reinterpret_cast<unsigned char*>(buff)
#define CCAST(buff) reinterpret_cast<const unsigned char*>(buff)

// Generate key pair.
void GenerateKeyPair(pkey_t* pkey, skey_t* skey) {
  crypto_box_keypair(pkey->data(), skey->data());
}

// Compute size of an onion cipher.
size_t CipherSize(party_id_t party_count) {
  size_t layers_count = party_count;
  size_t single_size = sizeof(OfflineSecret) + crypto_box_SEALBYTES;
  return single_size * layers_count;
}

// Onion encrypt.
std::unique_ptr<char[]> OnionEncrypt(const OfflineSecret* secrets,
                                     party_id_t first_party,
                                     party_id_t party_count,
                                     const std::vector<pkey_t>& pkeys) {
  // Compute how large the whole onion cipher is.
  party_id_t parties = party_count - first_party;
  size_t total_size = CipherSize(parties);

  // Allocate buffers for onion encryption.
  char* output_buffer = new char[total_size];
  char* input_buffer = new char[total_size];
  char* swap = nullptr;

  size_t offset = total_size;
  for (party_id_t i = parties; i > 0; i--) {
    party_id_t idx = i - 1;
    party_id_t party_id = first_party + idx;

    const unsigned char* secret =
        reinterpret_cast<const unsigned char*>(secrets + idx);

    offset -= sizeof(OfflineSecret);
    memcpy(input_buffer + offset, secret, sizeof(OfflineSecret));
    assert(crypto_box_seal(CAST(output_buffer) + offset - crypto_box_SEALBYTES,
                           CCAST(input_buffer) + offset, total_size - offset,
                           pkeys[party_id].data()) == 0);

    offset -= crypto_box_SEALBYTES;
    swap = output_buffer;
    output_buffer = input_buffer;
    input_buffer = swap;
  }

  // Sanity checks.
  assert(offset == 0);

  // Memory clean up
  delete[] output_buffer;
  return std::unique_ptr<char[]>(input_buffer);
}

// Onion decrypt (one layer).
OnionLayer OnionDecrypt(const char* cipher, party_id_t party_count,
                        const pkey_t& p, const skey_t& s) {
  // Compute how large the whole onion cipher is.
  size_t sz = CipherSize(party_count);
  char* plain = new char[sz - crypto_box_SEALBYTES];

  // Decrypt one layer.
  auto status =
      crypto_box_seal_open(CAST(plain), CCAST(cipher), sz, p.data(), s.data());
  assert(status == 0);

  return OnionLayer(std::unique_ptr<char[]>(plain));
}

}  // namespace onion
}  // namespace DPPIR
