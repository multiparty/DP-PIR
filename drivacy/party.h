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
#include "drivacy/protocol/query.h"
#include "drivacy/protocol/response.h"
#include "drivacy/types/config.pb.h"
#include "drivacy/types/types.h"

namespace drivacy {

template <typename S>
class Party {
  // Ensure S inherits from AbstractSocket.
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
  Party(uint32_t party_id, const types::Configuration &config,
        const types::Table &table)
      : party_id_(party_id), config_(config), table_(table) {
    this->socket_ = std::make_unique<S>(
        this->party_id_, absl::bind_front(&Party<S>::OnReceiveQuery, this),
        // virtual funtion binds to correct subclass.
        absl::bind_front(&Party<S>::OnReceiveResponse, this), this->config_);
  }

  virtual void Listen() { this->socket_->Listen(); }

 protected:
  uint32_t party_id_;
  const types::Configuration &config_;
  const types::Table &table_;
  std::unique_ptr<S> socket_;
  types::QueryState query_state_;

  // Called by the socket when a query is received.
  void OnReceiveQuery(const types::IncomingQuery &query) {
    if (this->party_id_ < this->config_.parties()) {
      // Process query.
      types::OutgoingQuery outgoing_query =
          protocol::query::ProcessQuery(this->party_id_, query, this->config_);
      // Store the query state.
      query_state_.preshare = outgoing_query.preshare();
      // Send the query over socket.
      this->socket_->SendQuery(outgoing_query);
    } else {
      // Process query creating a response, send it over socket.
      types::Response response =
          protocol::backend::QueryToResponse(query, config_, table_);
      this->socket_->SendResponse(response);
    }
  }

  // Called by the socket when a response is received.
  virtual void OnReceiveResponse(const types::Response &response) {
    uint64_t preshare = this->query_state_.preshare;
    types::Response outgoing_response =
        protocol::response::ProcessResponse(response, preshare);
    this->socket_->SendResponse(outgoing_response);
  }
};

// PartyHead is the special first party, which is responsible for communicating
// with other parties as well as clients!
template <typename S1, typename S2>
class PartyHead : public Party<S1> {
  // Ensure S1 and S2 inherit from AbstractSocket.
  static_assert(std::is_base_of<drivacy::io::socket::AbstractSocket, S1>::value,
                "S1 must inherit from AbstractSocket");
  static_assert(std::is_base_of<drivacy::io::socket::AbstractSocket, S2>::value,
                "S2 must inherit from AbstractSocket");

 public:
  PartyHead(PartyHead &&other) = delete;
  PartyHead &operator=(PartyHead &&other) = delete;
  PartyHead(const PartyHead &) = delete;
  PartyHead &operator=(const PartyHead &) = delete;

  PartyHead(uint32_t party, const types::Configuration &config,
            const types::Table &table)
      : Party<S1>(party, config, table) {
    this->client_socket_ = std::make_unique<S2>(
        this->party_id_,
        absl::bind_front(&PartyHead<S1, S2>::OnReceiveQuery, this),
        absl::bind_front(&PartyHead<S1, S2>::OnReceiveResponse, this),
        this->config_);
  }

  void Listen() override {
    Party<S1>::Listen();
    this->client_socket_->Listen();
  }

 protected:
  std::unique_ptr<S2> client_socket_;

  // Called by the socket when a response is received.
  void OnReceiveResponse(const types::Response &response) override {
    uint64_t preshare = this->query_state_.preshare;
    types::Response outgoing_response =
        protocol::response::ProcessResponse(response, preshare);
    this->client_socket_->SendResponse(outgoing_response);
  }
};

}  // namespace drivacy

#endif  // DRIVACY_PARTY_H_
