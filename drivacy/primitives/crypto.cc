// Copyright 2020 multiparty.org

// A wrapper for our crypto operations (public key encryption and signatures).

#include "drivacy/primitives/crypto.h"

#include <string>

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

void OnionEncrypt(const std::vector<types::QueryShare> &shares,
                  const types::Configuration &config,
                  types::Query *target_query) {
  // Compute how large the whole onion cipher is.
  size_t single_size = sizeof(types::QueryShare);
  size_t single_overhead = crypto_box_SEALBYTES;
  size_t total_size = (single_size + single_overhead) * shares.size();

  // Allocate buffers for onion encryption.
  unsigned char *onioncipher1 = new unsigned char[total_size];
  unsigned char *onioncipher2 = new unsigned char[total_size];

  size_t offset = total_size;
  size_t current_size = 0;
  for (size_t i = shares.size(); i > 0; i--) {
    const unsigned char *share =
        reinterpret_cast<const unsigned char *>(&shares.at(i - 1));
    const unsigned char *pk =
        ProtobufStringToByteBuffer(config.keys().at(i).public_key());

    offset = offset - single_size;
    memcpy(&onioncipher1[offset], share, single_size);
    assert(crypto_box_seal(onioncipher2, &onioncipher1[offset],
                           current_size + single_size, pk) == 0);

    current_size = current_size + single_size + single_overhead;
    offset = offset - single_overhead;
    memcpy(&onioncipher1[offset], onioncipher2, current_size);
  }

  // Sanity checks.
  assert(offset == 0);
  assert(current_size = total_size);

  // Set the result in the query.
  target_query->set_shares(onioncipher1, current_size);
}

types::QueryShare SingleLayerOnionDecrypt(uint32_t party_id,
                                          const types::Query &query,
                                          const types::Configuration &config,
                                          types::Query *target_query) {
  // Compute how large the whole onion cipher is.
  size_t single_size = sizeof(types::QueryShare);
  size_t single_overhead = crypto_box_SEALBYTES;
  size_t total_size =
      (single_size + single_overhead) * (config.parties() - party_id + 1);

  // Reinterpret the cipher string as a buffer.
  const unsigned char *onioncipher = ProtobufStringToByteBuffer(query.shares());

  // Allocate buffer for the plain text, which itself may contain more layers
  // of onion ciphers.
  size_t plain_size = total_size - single_overhead;
  unsigned char plain[plain_size];

  // Extract keys.
  const unsigned char *pk =
      ProtobufStringToByteBuffer(config.keys().at(party_id).public_key());
  const unsigned char *sk =
      ProtobufStringToByteBuffer(config.keys().at(party_id).secret_key());

  // Decrypt one layer.
  assert(crypto_box_seal_open(plain, onioncipher, total_size, pk, sk) == 0);

  // Extract plain text and inner layer onion cipher.
  if (target_query != nullptr) {
    target_query->set_shares(plain + single_size, plain_size - single_size);
  }

  return *reinterpret_cast<types::QueryShare *>(plain);
}

}  // namespace crypto
}  // namespace primitives
}  // namespace drivacy
