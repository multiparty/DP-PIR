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
    types::Response response;
    response.set_tag(query.tag());
    response.set_response(this->table_.at(next_tally));
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
  if (party == 1) {
    std::cout << "response: " << response.DebugString() << std::endl;
  } else {
    this->socket_->SendResponse(party - 1, response);
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
