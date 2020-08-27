// Copyright 2020 multiparty.org

// A wrapper for our crypto operations (public key encryption and signatures).

#ifndef DRIVACY_PRIMITIVES_CRYPTO_H_
#define DRIVACY_PRIMITIVES_CRYPTO_H_

#include <cstdint>
#include <vector>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/messages.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace primitives {
namespace crypto {

// Generate key pair for encryption.
types::KeyPair GenerateEncryptionKeyPair();

void OnionEncrypt(const std::vector<types::QueryShare> &shares,
                  const types::Configuration &config,
                  types::Query *target_query);

types::QueryShare SingleLayerOnionDecrypt(uint32_t party_id,
                                          const types::Query &query,
                                          const types::Configuration &config,
                                          types::Query *target_query);

}  // namespace crypto
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_CRYPTO_H_
