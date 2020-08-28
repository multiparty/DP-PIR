// Copyright 2020 multiparty.org

// This file defines the "Party" class, which represents
// A party/machine-specific instance of our protocol.
//
// When deployed, every machine's code will construct exactly a single
// instance of this class, and use it as the main control flow for the protocol.
//
// In a simulated environemnt, when all parties are run locally. The process
// will construct several instances of this "Party" class, one per logical
// party.

#ifndef DRIVACY_PARTY_H_
#define DRIVACY_PARTY_H_

#include <cstdint>
#include <memory>
#include <type_traits>

#include "absl/functional/bind_front.h"
#include "drivacy/io/abstract_socket.h"
#include "drivacy/protocol/backend.h"
#include "drivacy/protocol/client.h"
#include "drivacy/protocol/query.h"
#include "drivacy/protocol/response.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/messages.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {

template <typename S>
class Party {
  // Ensure S inherits AbstractSocket.
  static_assert(std::is_base_of<drivacy::io::socket::AbstractSocket, S>::value,
                "S must inherit from AbstractSocket");

 public:
  // Not movable or copyable: when an instance is constructed, a pointer to it
  // is stored in the socket (e.g. look at io/simulated_socket.[cc|h]).
  // If the object is later moved or copied, the pointer might become invalid.
  Party(Party &&other) = delete;
  Party &operator=(Party &&other) = delete;
  Party(const Party &) = delete;
  Party &operator=(const Party &) = delete;

  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  Party(uint32_t party, const types::Configuration &config,
        const types::Table &table)
      : party_id_(party), config_(config), table_(table), state_(party) {
    this->socket_ = std::make_unique<S>(
        this->party_id_, absl::bind_front(&Party<S>::OnReceiveQuery, this),
        absl::bind_front(&Party<S>::OnReceiveResponse, this));
  }

  void OnReceiveQuery(uint32_t party, const types::Query &query);
  void OnReceiveResponse(uint32_t party, const types::Response &response);
  void Start(uint64_t query);
  void End(const types::Response &response) const;

  uint32_t party_id() const { return this->party_id_; }

 private:
  uint32_t party_id_;
  const types::Configuration &config_;
  const types::Table &table_;
  std::unique_ptr<S> socket_;
  types::PartyState state_;
};

#include "drivacy/party.inc"

}  // namespace drivacy

#endif  // DRIVACY_PARTY_H_
