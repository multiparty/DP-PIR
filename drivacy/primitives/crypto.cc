// Copyright 2020 multiparty.org

// A wrapper for our crypto operations (public key encryption and signatures).

#include "drivacy/primitives/crypto.h"

#include "sodium.h"

namespace drivacy {
namespace primitives {
namespace crypto {

namespace {

inline unsigned char *ProtobufStringToByteBuffer(std::string *str) {
  return reinterpret_cast<unsigned char *>(&str->at(0));
}

}  // namespace

// Generate key pair for encryption.
types::KeyPair GenerateEncryptionKeyPair() {
  unsigned char pk[crypto_box_PUBLICKEYBYTES];
  unsigned char sk[crypto_box_SECRETKEYBYTES];
  crypto_box_keypair(pk, sk);

  types::KeyPair keypair;
  keypair.set_public_key(pk, crypto_box_PUBLICKEYBYTES);
  keypair.set_secret_key(sk, crypto_box_SECRETKEYBYTES);

  return keypair;
}

}  // namespace crypto
}  // namespace primitives
}  // namespace drivacy
