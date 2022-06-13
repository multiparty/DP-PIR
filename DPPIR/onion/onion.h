// Onion Encryption.
#ifndef DPPIR_ONION_ONION_H_
#define DPPIR_ONION_ONION_H_

#include <memory>
#include <utility>
#include <vector>

#include "DPPIR/types/types.h"

namespace DPPIR {
namespace onion {

// Generate key pair.
void GenerateKeyPair(pkey_t* pkey, skey_t* skey);

// Compute size of an onion cipher.
size_t CipherSize(party_id_t party_count);

// Onion encrypt.
std::unique_ptr<char[]> OnionEncrypt(const OfflineSecret* messages,
                                     party_id_t first_party,
                                     party_id_t party_count,
                                     const std::vector<pkey_t>& pkeys);

// Efficient onion decryption data structure:
// avoids an extra copy of the OfflineSecret and of the buffer, and
// keeps ownership/lifetime clear.
class OnionLayer {
 public:
  // Basically a wrapper around a buffer.
  explicit OnionLayer(std::unique_ptr<char[]>&& buf) : buf_(std::move(buf)) {}
  // Get components (no copy).
  OfflineSecret& Msg() {
    return *reinterpret_cast<OfflineSecret*>(this->buf_.get());
  }
  char* NextLayer() { return buf_.get() + sizeof(OfflineSecret); }

 private:
  std::unique_ptr<char[]> buf_;
};

// Onion decrypt (one layer).
OnionLayer OnionDecrypt(const char* cipher, party_id_t party_count,
                        const pkey_t& p, const skey_t& s);

}  // namespace onion
}  // namespace DPPIR

#endif  // DPPIR_ONION_ONION_H_
