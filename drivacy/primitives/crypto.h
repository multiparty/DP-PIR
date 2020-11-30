// Copyright 2020 multiparty.org

// A wrapper for our crypto operations (public key encryption and signatures).

#ifndef DRIVACY_PRIMITIVES_CRYPTO_H_
#define DRIVACY_PRIMITIVES_CRYPTO_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace primitives {
namespace crypto {

// Generate key pair for encryption.
types::KeyPair GenerateEncryptionKeyPair();

// The size of an onion ciher produced with:
// OnionEncrypt(..., ..., party_id);
// Size of the buffer read by party_id, and sent by party_id - 1.
uint32_t OnionCipherSize(uint32_t party_id, uint32_t party_count);

// Onion encrypt the given messages.
// party_id is the id of the party to which the first message belongs.
// so that messages[0]::onion[i] is encrypted with party i's key.
// while messages[1]::onion[i+1] is encrypted with party i+1's key.
// Precondition: party_id + messages.length() == config.parties() + 1
std::unique_ptr<unsigned char[]> OnionEncrypt(
    const std::vector<types::Message> &messages,
    const types::Configuration &config, uint32_t party_id);

types::OnionMessage SingleLayerOnionDecrypt(uint32_t party_id,
                                            const types::CipherText cipher,
                                            const types::Configuration &config);

}  // namespace crypto
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_CRYPTO_H_
