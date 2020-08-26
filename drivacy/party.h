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

#include <cstdlib>
#include <type_traits>

#include "absl/functional/bind_front.h"
#include "drivacy/io/abstract_socket.h"
#include "drivacy/primitives/secret_sharing.h"
#include "drivacy/proto/config.pb.h"
#include "drivacy/proto/messages.pb.h"
#include "drivacy/proto/table.pb.h"

namespace drivacy {

template <typename S>
class Party {
  // Ensure S inherits AbstractSocket.
  static_assert(std::is_base_of<drivacy::io::socket::AbstractSocket, S>::value,
                "S must inherit from AbstractSocket");

 public:
  Party(Party &&other) = default;
  Party &operator=(Party &&other) = default;
  Party(const Party &) = delete;
  Party &operator=(const Party &) = delete;

  Party(uint32_t party, const proto::Configuration &config,
        const proto::Table &table)
      : party_id_(party), config_(config), table_(table), tag_(0) {}

  void Configure();
  void OnReceiveQuery(uint32_t party, const proto::Query &query) const;
  void OnReceiveResponse(uint32_t party, const proto::Response &response) const;
  void Start();

  uint32_t party_id() const { return this->party_id_; }

 private:
  uint32_t party_id_;
  const proto::Configuration &config_;
  const proto::Table &table_;
  uint64_t tag_;
  S *socket_;
};

#include "drivacy/party.inc"

}  // namespace drivacy

#endif  // DRIVACY_PARTY_H_
