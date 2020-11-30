// Copyright 2020 multiparty.org

// Tests our incremental secret sharing implementation for correctness.
//
// Repeatedly comes up with random values in [0, prime), and secret shares
// them using our scheme, it then incrementally reconstructs the value
// from the shares, and tests that the final reconstruction is equal to the
// initial value.

#define PARTY_COUNT 5
#define TEST_COUNT 10

#include "drivacy/primitives/crypto.h"

#include <iostream>
#include <vector>
#include <memory>
#include <utility>

#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

std::vector<drivacy::types::Message> GenerateInput() {
  std::vector<drivacy::types::Message> result(PARTY_COUNT);
  for (uint32_t i = 0; i < PARTY_COUNT; i++) {
    drivacy::types::Message &msg = result.at(i);
    msg.tag = i;
    msg.reference.next_tag = i + 1;
    msg.reference.incremental_share.x = 10;
    msg.reference.incremental_share.y = i + 10;
    msg.reference.preshare = (i + 1) * 31;
  }
  return result;
}

bool Equals(const drivacy::types::OnionMessage &msg1,
            const drivacy::types::Message &msg2) {
  return msg1.tag() == msg2.tag && msg1.common_reference().next_tag == msg2.reference.next_tag && msg1.common_reference().incremental_share.y == msg2.reference.incremental_share.y && msg1.common_reference().preshare == msg2.reference.preshare;
}

bool Test(const drivacy::types::Configuration &config) {
  // Allocate input and output.
  std::vector<drivacy::types::Message> input = GenerateInput();
  std::vector<drivacy::types::OnionMessage> output;

  // Onion encrypt.
  for (uint32_t party_id = 1; party_id <= PARTY_COUNT; party_id++) {
    std::unique_ptr<unsigned char[]> encrypted =
        drivacy::primitives::crypto::OnionEncrypt(input, config, party_id);

    // Onion decrypt.
    drivacy::types::CipherText onion = encrypted.get();
    for (uint32_t i = party_id; i <= PARTY_COUNT; i++) {
      drivacy::types::OnionMessage message = 
          drivacy::primitives::crypto::SingleLayerOnionDecrypt(i, onion, config);
      onion = message.onion_cipher();
      if (!Equals(message, input.at(i - party_id))) {
        std::cout << "Message decrypted wrong!" << std::endl;
        return false;
      }

      // Do not destruct message yet...
      output.push_back(std::move(message));
    }

    // Smaller input.
    input.pop_back();
  }

  return true;
}

int main() {
  // Build configuration prototype.
  drivacy::types::Configuration config;
  config.set_parties(PARTY_COUNT);
  for (uint32_t party_id = 1; party_id <= PARTY_COUNT; party_id++) {
    config.mutable_keys()->insert(
        {party_id, drivacy::primitives::crypto::GenerateEncryptionKeyPair()});
  }

  // Perform tests.
  for (uint32_t i = 0; i < TEST_COUNT; i++) {
    if (!Test(config)) {
      return 1;
    }
  }

  std::cout << "All tests passed!" << std::endl;
  return 0;
}
