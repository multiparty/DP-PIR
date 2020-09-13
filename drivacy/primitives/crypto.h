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

void OnionEncrypt(const std::vector<types::QueryShare> &shares,
                  const types::Configuration &config, unsigned char *cipher);

void SingleLayerOnionDecrypt(uint32_t party_id, const unsigned char *cipher,
                             const types::Configuration &config,
                             unsigned char *plain);

}  // namespace crypto
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_CRYPTO_H_
