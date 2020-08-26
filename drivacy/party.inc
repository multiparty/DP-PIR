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
void Party<S>::OnReceiveQuery(uint32_t party, const types::Query &query) const {
  // Find next tally.
  const types::QueryShare &share = query.shares(0);
  uint64_t next_tally =
      primitives::IncrementalReconstruct(query.tally(), {share.x(), share.y()});

  if (this->party_id_ == this->config_.parties()) {
    std::cout << "query: " << next_tally << std::endl;
    uint64_t response_value = this->table_.at(next_tally);
    auto shares = primitives::GenerateAdditiveSecretShares(
        response_value, this->config_.parties());

    types::Response response;
    response.set_tag(query.tag());
    response.set_tally(0);
    for (const auto &share : shares) {
      response.add_shares(share);
    }
    this->socket_->SendResponse(party, response);
  } else {
    types::Query next_query;
    next_query.set_tag(query.tag());
    next_query.set_tally(next_tally);
    // Discard the first share that was already used, keep the next shares.
    auto iterator = query.shares().cbegin();
    for (iterator++; iterator != query.shares().cend(); iterator++) {
      types::QueryShare *next_share = next_query.add_shares();
      next_share->set_x(iterator->x());
      next_share->set_y(iterator->y());
    }
    // Send the query to the next party.
    this->socket_->SendQuery(this->party_id_ + 1, next_query);
  }
}

template <typename S>
void Party<S>::OnReceiveResponse(uint32_t party,
                                 const types::Response &response) const {
  // Find next tally.
  uint64_t share = response.shares(0);
  uint64_t next_tally =
      primitives::AdditiveReconstruct(response.tally(), share);

  // Construct next response object.
  types::Response next_response;
  next_response.set_tag(response.tag());
  next_response.set_tally(next_tally);
  auto iterator = response.shares().cbegin();
  for (iterator++; iterator != response.shares().cend(); iterator++) {
    next_response.add_shares(*iterator);
  }

  // Send next response to the following party or client.
  if (this->party_id_ == 1) {
    this->End(next_response);
  } else {
    this->socket_->SendResponse(this->party_id_ - 1, next_response);
  }
}

template <typename S>
void Party<S>::Start(uint64_t q) {
  types::Query query;
  query.set_tag(this->tag_++);
  query.set_tally(1);
  auto shares =
      primitives::GenerateIncrementalSecretShares(q, this->config_.parties());
  for (const auto &share : shares) {
    types::QueryShare *query_share = query.add_shares();
    query_share->set_x(share.x);
    query_share->set_y(share.y);
  }
  this->OnReceiveQuery(0, query);
}

template <typename S>
void Party<S>::End(const types::Response &response) const {
  uint64_t share = response.shares(0);
  uint64_t next_tally =
      primitives::AdditiveReconstruct(response.tally(), share);
  std::cout << "response: " << next_tally << std::endl;
}
