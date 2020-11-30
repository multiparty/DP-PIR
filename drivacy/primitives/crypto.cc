// Copyright 2020 multiparty.org

// A wrapper for our crypto operations (public key encryption and signatures).

#include "drivacy/primitives/crypto.h"

#include <cstring>
#include <string>

// NOLINTNEXTLINE
#include "sodium.h"

namespace drivacy {
namespace primitives {
namespace crypto {

namespace {

inline const unsigned char *ProtobufStringToByteBuffer(const std::string &str) {
  return reinterpret_cast<const unsigned char *>(&str.at(0));
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

uint32_t OnionCipherSize(uint32_t party_id, uint32_t party_count) {
  uint32_t layers_count = party_count - party_id + 1;
  uint32_t single_size = sizeof(types::Message) + crypto_box_SEALBYTES;
  return single_size * layers_count;
}

std::unique_ptr<unsigned char[]> OnionEncrypt(
    const std::vector<types::Message> &messages,
    const types::Configuration &config, uint32_t party_id) {
  assert(messages.size() + party_id == config.parties() + 1);

  // Compute how large the whole onion cipher is.
  size_t total_size = OnionCipherSize(party_id, config.parties());

  // Allocate buffers for onion encryption.
  unsigned char *onioncipher1 = new unsigned char[total_size];
  unsigned char *onioncipher2 = new unsigned char[total_size];
  unsigned char *swap = nullptr;

  size_t offset = total_size;
  for (size_t i = messages.size(); i > 0; i--) {
    const unsigned char *msg =
        reinterpret_cast<const unsigned char *>(&messages.at(i - 1));
    const unsigned char *pk = ProtobufStringToByteBuffer(
        config.keys().at(i + party_id - 1).public_key());

    offset = offset - sizeof(types::Message);
    memcpy(onioncipher1 + offset, msg, sizeof(types::Message));
    assert(crypto_box_seal(onioncipher2 + offset - crypto_box_SEALBYTES,
                           onioncipher1 + offset,
                           total_size - offset, pk) == 0);

    offset = offset - crypto_box_SEALBYTES;
    swap = onioncipher1;
    onioncipher1 = onioncipher2;
    onioncipher2 = swap;
  }

  // Sanity checks.
  assert(offset == 0);

  // Memory clean up
  delete[] onioncipher2;

  return std::unique_ptr<unsigned char[]>(onioncipher1);
}

types::OnionMessage SingleLayerOnionDecrypt(uint32_t party_id,
                                            const types::CipherText cipher,
                                            const types::Configuration &config) {
  // Compute how large the whole onion cipher is.
  size_t cipher_size = OnionCipherSize(party_id, config.parties());
  size_t plain_size = cipher_size - crypto_box_SEALBYTES;
  unsigned char *onionplain = new unsigned char[plain_size];

  // Extract keys.
  const unsigned char *pk =
      ProtobufStringToByteBuffer(config.keys().at(party_id).public_key());
  const unsigned char *sk =
      ProtobufStringToByteBuffer(config.keys().at(party_id).secret_key());

  // Decrypt one layer.
  assert(crypto_box_seal_open(onionplain, cipher, cipher_size, pk, sk) == 0);
  return types::OnionMessage(std::unique_ptr<unsigned char[]>(onionplain));
}

}  // namespace crypto
}  // namespace primitives
}  // namespace drivacy
