// Copyright 2020 multiparty.org

// A wrapper for our crypto operations (public key encryption and signatures).

#ifndef DRIVACY_PRIMITIVES_CRYPTO_H_
#define DRIVACY_PRIMITIVES_CRYPTO_H_

#include "drivacy/types/config.pb.h"

namespace drivacy {
namespace primitives {
namespace crypto {

// Generate key pair for encryption.
types::KeyPair GenerateEncryptionKeyPair();

}  // namespace crypto
}  // namespace primitives
}  // namespace drivacy

#endif  // DRIVACY_PRIMITIVES_CRYPTO_H_
