// Copyright 2020 multiparty.org

// This file defines the "Client" class, which represents
// a single client in our protocol. The client is responsible for creating
// queries and handling responses.
//
// When deployed, this client communicates with the appropriate (server) parties
// on the wire. In a simulated enviornment, the client and parties are run
// locally in the same process, and they interact directly via the enclosing
// simulation code.

#include "drivacy/parties/offline/client.h"

#include <memory>
#include <utility>

#include "drivacy/primitives/crypto.h"
#include "drivacy/protocol/offline/client.h"

namespace drivacy {
namespace parties {
namespace offline {

void Client::Subscribe(uint32_t count) {
  // TODO(babman): do not hard code this.
  uint32_t total_per_machine = count * 20;
  uint32_t seed = (this->machine_id_ - 1) * total_per_machine +
                  (this->client_id_ - 1) * count;
  for (uint32_t i = 0; i < count; i++) {
    // Sample random tags and shares.
    this->common_references_.push_back(
        std::move(protocol::offline::client::SampleCommonReference(
            seed++, 0, this->config_.parties())));
    // Onion encrypted sampled randomness.
    std::unique_ptr<unsigned char[]> cipher = primitives::crypto::OnionEncrypt(
        this->common_references_.back(), this->config_, 1);
    // Send cipher over socket.
    this->socket_.SendMessage(cipher.get());
  }
  this->SaveCommonReference();
}

void Client::SaveCommonReference() {
  for (const auto &nested : this->common_references_) {
    for (const auto &tuple : nested) {
      std::cout << tuple.tag << std::endl;
      std::cout << tuple.reference.next_tag << " "
                << tuple.reference.incremental_share.x << " "
                << tuple.reference.incremental_share.y << " "
                << tuple.reference.preshare << std::endl;
    }
  }
}

}  // namespace offline
}  // namespace parties
}  // namespace drivacy
