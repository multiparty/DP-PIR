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
#include <thread>
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
  Party(uint32_t party, const types::Configuration &config,
        const types::Table &table)
      : party_id_(party), config_(config), table_(table), state_(party) {
    this->socket_ = std::make_unique<S>(
        this->party_id_, absl::bind_front(&Party<S>::OnReceiveQuery, this),
        // virtual funtion binds to correct subclass.
        absl::bind_front(&Party<S>::OnReceiveResponse, this));
  }

  virtual void Listen() { this->socket_->Listen(this->config_); }
  virtual void Close() { this->socket_->Close(); }

 protected:
  // Called by the socket when a query is received.
  void OnReceiveQuery(uint32_t party, const types::Query &query) {
    if (this->party_id_ == this->config_.parties()) {
      types::Response response = protocol::backend::QueryToResponse(
          query, this->config_, this->table_, &this->state_);
      this->socket_->SendResponse(this->party_id_ - 1, response);
    } else {
      types::Query next_query =
          protocol::query::ProcessQuery(query, this->config_, &this->state_);
      this->socket_->SendQuery(this->party_id_ + 1, next_query);
    }
  }

  // Called by the socket when a response is received.
  virtual void OnReceiveResponse(uint32_t party,
                                 const types::Response &response) {
    types::Response next_response =
        protocol::response::ProcessResponse(response, &this->state_);
    this->socket_->SendResponse(this->party_id_ - 1, next_response);
  }

  uint32_t party_id_;
  const types::Configuration &config_;
  const types::Table &table_;
  std::unique_ptr<S> socket_;
  types::PartyState state_;
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
        absl::bind_front(&PartyHead<S1, S2>::OnReceiveResponse, this));
  }

  void Listen() override {
    std::thread t([this] { Party<S1>::Listen(); });
    this->client_socket_->Listen(this->config_);
  }

  void Close() override {
    Party<S1>::Close();
    this->client_socket_->Close();
  }

 protected:
  // Called by the socket when a response is received.
  void OnReceiveResponse(uint32_t party,
                         const types::Response &response) override {
    types::Response next_response =
        protocol::response::ProcessResponse(response, &this->state_);
    this->client_socket_->SendResponse(next_response.tag(), next_response);
  }

  std::unique_ptr<S2> client_socket_;
};

}  // namespace drivacy

#endif  // DRIVACY_PARTY_H_
