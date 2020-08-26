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

template <typename S>
void Party<S>::Configure() {
  this->socket_ =
      new S(this->party_id_, absl::bind_front(&Party<S>::OnReceiveQuery, this),
            absl::bind_front(&Party<S>::OnReceiveResponse, this));
}

template <typename S>
void Party<S>::OnReceiveQuery(uint32_t party, const types::Query &query) {
  if (this->party_id_ == this->config_.parties()) {
    types::Response response =
        protocol::backend::QueryToResponse(query, this->table_);
    this->socket_->SendResponse(this->party_id_ - 1, response);
  } else {
    types::Query next_query =
        protocol::query::ProcessQuery(query, &this->state_);
    this->socket_->SendQuery(this->party_id_ + 1, next_query);
  }
}

template <typename S>
void Party<S>::OnReceiveResponse(uint32_t party,
                                 const types::Response &response) {
  types::Response next_response =
      protocol::response::ProcessResponse(response, &this->state_);
  if (this->party_id_ == 1) {
    this->End(next_response);
  } else {
    this->socket_->SendResponse(this->party_id_ - 1, next_response);
  }
}

template <typename S>
void Party<S>::Start(uint64_t q) {
  types::Query query =
      protocol::client::CreateQuery(q, this->config_.parties());
  this->OnReceiveQuery(0, query);
}

template <typename S>
void Party<S>::End(const types::Response &response) const {
  protocol::client::ReconstructResponse(response);
}
