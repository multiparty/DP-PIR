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

#ifndef DRIVACY_PARTIES_PARTY_H_
#define DRIVACY_PARTIES_PARTY_H_

#include <cstdint>
#include <memory>

#include "absl/functional/bind_front.h"
#include "drivacy/io/abstract_socket.h"
#include "drivacy/protocol/shuffle.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {
namespace parties {

class Party {
 public:
  // Construct the party given its configuration.
  // Creates a socket of the appropriate template type with an internal
  // back-pointer to the party.
  Party(uint32_t party_id, const types::Configuration &config,
        const types::Table &table, io::socket::SocketFactory socket_factory)
      : party_id_(party_id), config_(config), table_(table) {
    // virtual funtion binds to correct subclass.
    this->socket_ = socket_factory(
        this->party_id_, absl::bind_front(&Party::OnReceiveQuery, this),
        absl::bind_front(&Party::OnReceiveResponse, this), this->config_);

    this->size_ = 3;
    this->shuffler_.Initialize(this->size_);
  }

  // Not movable or copyable: when an instance is constructed, a pointer to it
  // is stored in the socket (e.g. look at io/simulated_socket.[cc|h]).
  // If the object is later moved or copied, the pointer might become invalid.
  Party(Party &&other) = delete;
  Party &operator=(Party &&other) = delete;
  Party(const Party &) = delete;
  Party &operator=(const Party &) = delete;

  // Called to start the listening on the socket (blocking!)
  virtual void Listen();

 protected:
  uint32_t party_id_;
  const types::Configuration &config_;
  const types::Table &table_;
  std::unique_ptr<io::socket::AbstractSocket> socket_;
  protocol::Shuffler shuffler_;
  uint32_t size_;

  // Called by the socket when a query is received.
  void OnReceiveQuery(const types::IncomingQuery &query);
  // Called by the socket when a response is received.
  virtual void OnReceiveResponse(const types::Response &response);
};

}  // namespace parties
}  // namespace drivacy

#endif  // DRIVACY_PARTIES_PARTY_H_
