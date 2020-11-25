// Copyright 2020 multiparty.org

// A wrapper for our crypto operations (public key encryption and signatures).

#ifndef DRIVACY_PRIMITIVES_CRYPTO_H_
#define DRIVACY_PRIMITIVES_CRYPTO_H_

#include <cstdint>
#include <vector>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace primitives {
namespace crypto {

// Generate key pair for encryption.
types::KeyPair GenerateEncryptionKeyPair();

uint32_t OnionCipherSize(uint32_t party_id, uint32_t party_count);

// Onion encrypt the given shares into the cipher buffer.
// party_id is the id of the party to which the first share belongs.
// so that shares[0]::onion[i] is encrypted with party i's key.
// while shares[1]::onion[i+1] is encrypted with party i+1's key.
// Precondition: party_id + shares.length() == config.parties() + 1
void OnionEncrypt(const std::vector<types::QueryShare> &shares,
                  const types::Configuration &config, unsigned char *cipher,
                  uint32_t party_id);

void SingleLayerOnionDecrypt(uint32_t party_id, const unsigned char *cipher,
                             const types::Configuration &config,
                             unsigned char *plain);

}  // namespace crypto
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_CRYPTO_H_
